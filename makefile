#makes a separate client and server program

LDFLAGS = -pthread
program : chatclient chatserver

#target to make client
client: chatclient.o
	gcc `./pflags` -o chatclient chatclient.o


#target to make server
server: chatserver.o
	gcc `./pflags` -o chatserver chatserver.o $(LDFLAGS)


#client object dependencies
chatclient.o: chatclient.c
	gcc `./pflags` -c chatclient.c


#server object dependencies
chatserver.o: chatserver.c
	gcc `./pflags` -c  chatserver.c


clean:
	@rm -f chatclient.o chatserver.o chatclient chatserver
