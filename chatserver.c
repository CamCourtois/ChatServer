#include <sys/socket.h> //socket(), bind(), listen(), accept()
#include <stdlib.h>		//exit()
#include <unistd.h>		//send(), close(), read(), write()
#include <arpa/inet.h>	//htons()
#include <string.h>		//memset(), //strlen()
#include <stdio.h>		//printf()
#include <pthread.h>	//POSIX Threads

#define TEMP_PORT 12346
#define BUF_SIZE 1024
#define NUM_THREADS 10
#define NUM_CLIENTS 4

/*****  MUTEX LOCK FOR STRUCT***/
pthread_mutex_t client_args_lock = PTHREAD_MUTEX_INITIALIZER;

// global array to hold up to 4 unique connected users, 15 characters each
char user_array[NUM_CLIENTS][15];

// global array to hold 4 client file descriptors
int client_fds[NUM_CLIENTS];

// client thread function declaration
void *clientThreadFunc(void *);

struct thread_args
{

	char clientname[15];
	int clientfd;

} client_args;

char readbuf[BUF_SIZE];

// MAIN: server
int main(void)
{
	pthread_t threads[NUM_THREADS];
	void *result; // return from thread
	int listen_fd, client_fd, err, opt, i, j;
	ssize_t wcount, rcount;
	socklen_t addrlen;
	int port = TEMP_PORT;
	struct sockaddr_in addr;

	char *errorMsg = "sorry, username is already taken\0";
	char *successMsg = "username is available!\0";
	char *serverFull = "Sorry, chat server is currently full";
	int connectionStatus = 1;

	int client_connections = 0; // track num clients connected

	char *msg;
	msg = (char *)malloc(50 * sizeof(char)); // allocate space for a server msg of 50 characters

	// initialize user_array to empty strings
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		for (j = 0; j < 15; j++)
		{
			user_array[i][j] = '\0';
		}
	}

	// initalize client_fds array to -1 to indicate unoccupied client
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		client_fds[i] = -1;
	}

	// initialize array of empty thread to 0 to indicate not running
	for (i = 0; i < NUM_THREADS; i++)
	{
		threads[i] = 0;
	}

	// create a new internet domain stream socket (TCP/IP socket)
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	// exit on socket() error
	if (listen_fd == -1)
	{
		fprintf(stderr, "\nsocket(): exiting server socket\n");
		exit(1);
	}

	// sets socket opetions, lets server quickly rebind to a partly closed port
	opt = 1;
	err = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// set addr struct 0 bytes, zero out unused fields
	memset(&addr, 0, sizeof(struct sockaddr_in));

	// set addr structure members
	addr.sin_family = AF_INET;		   // IPv4 addr type
	addr.sin_addr.s_addr = INADDR_ANY; // bind to any local address
	addr.sin_port = htons(port);	   // convert port to network byte ordering

	// store the struct size
	addrlen = sizeof(addr);

	// bind socket to TEMP_PORT(12346)
	err = bind(listen_fd, (struct sockaddr *)&addr, addrlen);

	// exit on bind() error
	if (err == -1)
	{
		close(listen_fd);
		fprintf(stderr, "\nbind(): exiting\n\n");
		exit(2);
	}

	// begin listening for connections w/ a max backlog of 1
	err = listen(listen_fd, 32);

	// exit on listen() error
	if (err == -1)
	{
		close(listen_fd);
		fprintf(stderr, "\nlisten(): exiting\n");
		exit(3);
	}

	printf("\nWaiting for client to connect... \n\n");

	// allocate 15 char storage for username and servermsg fields
	char *given_username;
	given_username = (char *)malloc(15 * sizeof(char));
	char *initial_servermsg;
	initial_servermsg = (char *)malloc(50 * sizeof(char));
	int servermsg;

	// accept client connections
	//  this will be done in a loop to accept NUM_CLIENTS into server
	while (1)
	{

		client_fd = accept(listen_fd, NULL, NULL);

		if (client_connections == NUM_CLIENTS)
		{
			write(client_fd, serverFull, strlen(serverFull));
			close(client_fd);
			continue;
		}

		// exit on accept() error
		if (client_fd <= 0)
		{
			close(listen_fd);
			fprintf(stderr, "\naccept(): exiting\n");
			exit(4);
		}

		// receive client username
		rcount = read(client_fd, readbuf, 15);
		readbuf[rcount] = '\0';
		strcpy(given_username, readbuf);

		// check if username is taken, if not, store it, else throw an error
		for (i = 0; i < NUM_CLIENTS; i++)
		{
			if (user_array[i][0] == '\0')
			{
				send(client_fd, successMsg, strlen(errorMsg), 0);
				strcpy(user_array[i], given_username);
				memset(readbuf, 0, BUF_SIZE);
				break;
			}
			else if (strcmp(user_array[i], readbuf) == 0)
			{
				send(client_fd, errorMsg, strlen(errorMsg), 0);
				close(client_fd);
				connectionStatus = 0;
				break;
			}
		}

		// confirm to client that they are connected server
		if (connectionStatus == 1)
		{
			printf("<%s> successfully connected to chatserver\n", given_username);

			sprintf(initial_servermsg, "<%s> is connected to chat server\n", given_username);
			servermsg = send(client_fd, initial_servermsg, strlen(initial_servermsg), 0);

			if (servermsg < 0)
			{
				fprintf(stderr, "\nerror writing from server\n");
				exit(1);
			}

			// temporarily store client fd and username to be passed to thread function when thread is created
			strcpy(client_args.clientname, given_username);
			client_args.clientfd = client_fd;

			// add client to global client array
			for (i = 0; i < NUM_CLIENTS; i++)
			{
				if (client_fds[i] == -1)
				{
					client_fds[i] = client_fd;
					break;
				}
			}

			// send message to client after connecting
			wcount = write(client_fd, msg, strlen(msg));

			// exit on EOF error
			if (wcount < (int)strlen(msg))
			{
				close(listen_fd);
				close(client_fd);
				fprintf(stderr, "\nwrite(): exiting\n");
				exit(5);
			}

			for (int k = 0; k < NUM_THREADS; k++)
			{

				// check if thread is unused, if it is assign it to the current client
				if (threads[i] == 0)
				{
					printf("\nthread is unused; creating a client thread for <%s>\n", given_username);

					pthread_create(&threads[i], NULL, clientThreadFunc, (void *)&client_args);
				}
			}
		}
		client_connections++;
	}

	if (client_connections == 3)
	{
		fprintf(stderr, "\nmaximum clients connected");
	}

	// close all server threads
	for (i = 0; i < 10; i++)
	{
		pthread_join(threads[i], &result);
	}

	// exit by closing sockets

	// close client socket
	close(client_fd);

	// close listen socket
	close(listen_fd);

	exit(EXIT_SUCCESS);
}

void *clientThreadFunc(void *arg)
{

	// cast thread arg back to struct thread type
	struct thread_args *args = (struct thread_args *)arg;

	// lock client_args struct
	pthread_mutex_lock(&client_args_lock);

	// access client thread args
	char *user_name = args->clientname;
	int client_fd = args->clientfd;

	// unlock client_args struct
	pthread_mutex_unlock(&client_args_lock);

	printf("\n<%s> is ready to chat; and has a fd of: %d>\n", user_name, client_fd);

	char received_msg_buf[BUF_SIZE];

	// listen for messages from client
	int clientread;
	while ((clientread = read(client_fd, received_msg_buf, BUF_SIZE)) > 0)
	{
		for (int i = 0; i < NUM_CLIENTS; i++) // while num clients is less than maximum
		{
			if (client_fds[i] != -1) // if the client fd currently occupied, send them the message
			{
				char msg[BUF_SIZE + 20];
				sprintf(msg, " %s", received_msg_buf);

				write(client_fds[i], msg, strlen(msg));
			}
		}
		memset(received_msg_buf, 0, BUF_SIZE); // reset the buffer for later use
	}
	return (void *)strlen(user_name);
}
