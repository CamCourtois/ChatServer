#include <sys/socket.h> //socket(), connect()
#include <stdlib.h>		//exit()
#include <unistd.h>		//send(), close(), read(), write()
#include <arpa/inet.h>	//htons()
#include <string.h>		//memset()
#include <stdio.h>		//printf()
#include <pthread.h>	//threads

#define TEMP_PORT 12346
#define BUF_SIZE 1024

// declare read, user_write_thread function
void *read_server_messages(void *arg);
void *write_user_input(void *arg);

// global variables to use later
int server_fd;
pthread_t reading_thread;
pthread_t user_input_thread;

// MAIN: client
int main(void)
{
	int err;
	ssize_t rcount, wcount;
	socklen_t addrlen;
	int port = TEMP_PORT;
	struct sockaddr_in addr;
	char readbuf[BUF_SIZE];

	// allocate a 15 character space for a username
	char *username;
	username = (char *)malloc(15 * sizeof(char));

	// allocate a 50 character space for a chat message
	char *client_msg;
	client_msg = (char *)malloc(50 * sizeof(char));

	// create a new stream socket (TCP/IP socket)
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Exit on socket() error
	if (server_fd == -1)
		exit(1);

	// set addr struct to 0 bytes (to zero out unused fields)
	memset(&addr, 0, sizeof(struct sockaddr_in));

	// set addr struct members
	addr.sin_family = AF_INET;		   // IPv4 addr type
	addr.sin_addr.s_addr = INADDR_ANY; // bind to any local port
	addr.sin_port = htons(port);	   // convert port to network byte ordering

	// store struct size
	addrlen = sizeof(addr);

	printf(" \n");
	printf("---------------------------------------------------------\n");
	printf("Welcome to the chat server!\n");
	printf("Please enter a username (max 15 characters)\n");
	printf("----------------------------------------------\n");

	fgets(username, 15, stdin); // read up to 15 characters including spaces from stdin

	// remove newline character from username input
	char *newline = strchr(username, '\n');
	if (newline)
	{
		*newline = '\0';
	}
	printf("Entered username: %s\n", username);

	// attempt a connection to the server
	err = connect(server_fd, (struct sockaddr *)&addr, addrlen);

	// close socket and exit on connect() error
	if (err == -1)
	{
		close(server_fd);
		exit(2);
	}

	// send username to server
	wcount = write(server_fd, username, strlen(username));

	// exit on write error
	if (wcount < (int)strlen(username))
	{
		close(server_fd);

		fprintf(stderr, "\nwrite(): exiting\n");
		exit(5);
	}

	// check to see if server sent a message
	rcount = read(server_fd, readbuf, BUF_SIZE);

	// close socket and exit on read error or EOF
	if ((rcount == -1) || (rcount == 0))
	{
		close(server_fd);
		exit(3);
	}

	// print contents of readbuf received from server
	printf("\n%s\n", readbuf);
	printf("\n");
	if (strncmp(readbuf, "Sorry", 5) == 0)
	{
		printf("Closing connection....\n");
		close(server_fd);
		exit(1);
	}
	else
	{
		printf("<%s> is ready to begin chatting!\n", username);
		printf("---------------------------------------------------------\n");

		// create thread to read server messages
		pthread_create(&reading_thread, NULL, read_server_messages, NULL);

		// create thread to handle userinput
		pthread_create(&user_input_thread, NULL, write_user_input, username);

		// join read/user write thread and exit
		pthread_join(reading_thread, NULL);
		pthread_join(user_input_thread, NULL);

		// close client socket and exit
		close(server_fd);

		exit(1);

		printf("---------------------------------------------------------\n");
	}
}

void *write_user_input(void *arg)
{
	char *username = (char *)arg;

	// allocate a 50 character space for a chat message
	char *client_msg;
	client_msg = (char *)malloc(50 * sizeof(char));

	// read write loop from client input
	while (1)
	{
		char full_message[BUF_SIZE + 24];

		// read client message input from stdin
		fgets(client_msg, BUF_SIZE, stdin);

		// check if client message being with  \, if so check if it 'q' follows, if yes, exit server
		if (client_msg[0] == '\\')
		{
			if (client_msg[1] == 'q')
			{
				printf("				\n");
				printf("Exiting chat server..........\n");

				// let other clients in server know you have left
				sprintf(full_message, "<%s>: has left the chat server.", username);
				int len = strlen(full_message);
				full_message[len] = '\0';
				write(server_fd, full_message, (ssize_t)len + 1); // include null terminator in msg length

				printf("---------------------------------------------------------\n");
				pthread_cancel(reading_thread);
				pthread_exit(NULL);
			}
			else
			{
				printf("Invalid command. Type '\\q' to exit the chat.\n");
			}
		}

		// remove newline character from client message input
		char *newline = strchr(client_msg, '\n');
		if (newline)
		{
			*newline = '\0';
		}
		sprintf(full_message, "<%s>: %s", username, client_msg); // format the sent server message w/ client username

		int len = strlen(full_message);
		full_message[len] = '\0';
		write(server_fd, full_message, (ssize_t)len + 1); // include null terminator in msg length
	}
}

void *read_server_messages(void *arg)
{
	char active_readbuf[BUF_SIZE];

	while (1)
	{
		ssize_t rcount = read(server_fd, active_readbuf, BUF_SIZE);

		if (rcount <= 0)
		{
			// error or EOF from server exit thread
			break;
		}
		printf("%s\n", active_readbuf);
		memset(active_readbuf, 0, BUF_SIZE);
	}

	// close client socket
	close(server_fd);

	exit(1);
}