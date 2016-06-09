#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <pthread.h>

#include "networkdef.h"

/* Connection status: 1 if connected with a server, 0 elsewhere */
int connected;
/* Socket file descriptor of the connection with the server */
int serversfd;
/* Alias of the client */
char myalias[ALIASLEN];

/* Routine that constantly listens for incoming packets.
The method returns a void pointer because it is needed to create a thread */
void *receiver() {
	struct Packet packet; // this packet will be used to contain the received data
	printf("DEBUG: Listener set [%d]\n", serversfd);
	while(1) {
		if(!(recv(serversfd, (void *)&packet, sizeof(struct Packet), 0))) {
			/* When recv returns 0, it means that the connection was interrupted */
			fprintf(stderr, "client: connection lost from server\n");
			connected = 0;
			close(serversfd);
			break;
		}
		/* Display the message on screen. TODO: create a dedicate method */
		printf("[%s]: %s\n", packet.alias, packet.payload);
		/* Clean the packet */
		memset(&packet, 0, sizeof(struct Packet));
	}
	return NULL;
}

/* Establish a connection with the server using SERVERIP and SERVERPORT.
Return -1 if an error occurred, the socket file descriptor elsewhere */
int connect_server() {
	/* Prepare the server's address */
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	if ((status = getaddrinfo(SERVERIP, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		return -1;
	}

	/* open a socket */
	int newfd;
	if((newfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		perror("client: socket");
		return -1;
	}
	/* try to connect with server */
	if(connect(newfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		perror("client: socket");
		return -1;
	}
	else {
		printf("client: connected to server at %s:%s\n", SERVERIP, SERVERPORT);
		return newfd;
	}
	freeaddrinfo(servinfo); // free the linked-list
	freeaddrinfo(&hints);
	return 0;
}

/* Send a request to the server to change your own alias to name */
int setalias(char name[]) {
	if(!connected) {
		fprintf(stderr, "client: you are not connected\n");
		return -1;
	}

	/* Prepare the packet */
	struct Packet packet;
	memset(&packet, 0, sizeof(struct Packet));
	packet.action = ALIAS;
	strcpy(packet.alias, name);
	/* Send the packet */
	if(send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
	return 0;
}

/* Login to the server, if name is not NULL, the alias is set accordingly */
int login(char name[]) {
	int recvd;
	if (connected) {
		fprintf(stderr, "client: you are already connected to server at %s:%s\n", SERVERIP, SERVERPORT);
		return -1;
	}
	/* Temporary variable containing the socket file declarator of the connection */
	int sockfd;
	if ((sockfd = connect_server()) == -1) {
		fprintf(stderr, "client: error occurred in method connect_server\nclient: connection failed\n");
		return -1;
	}
	if(sockfd >= 0) {
		connected = 1;
		serversfd = sockfd;
		if(name != NULL) {
			setalias(name);
		}
		printf("client: logged in as %s\n", name);
		/* Start a thread to handle the packages' reception */
		pthread_t receive;
		pthread_create(&receive, NULL, receiver, NULL);
	}
	else {
		fprintf(stderr, "client: connection failed\n");
		return -1;
	}
	return 0;
}

/* Send a message to a client specifying his alias */
int send_msg(char target[], char msg[]) {
	int sent, targetlen;
	struct Packet packet;

	if(target == NULL || msg == NULL) {
		return 0;
	}

	if(!connected) {
		fprintf(stderr, "client: you are not connected\n");
		return -1;
	}

	/* Build the packet */
	memset(&packet, 0, sizeof(struct Packet)); // make sure the packet is clean
	packet.action = WHISPER;
	strcpy(packet.alias, myalias);
	/* In the packet's payload insert the target's alias and the message */
	strcpy(packet.payload, target);
	targetlen = strlen(target);
	strcpy(&packet.payload[targetlen], " "); // add a space to separate the target from the message's body
	strcpy(&packet.payload[targetlen+1], msg);
	printf("DEBUG: the packet's payload is %s\n", packet.payload);
	/* Send the packet */
	if (send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
	return 0;
}

/* Interrupt the connection with the server */
int logout() {
	struct Packet packet;

	if(!connected) {
		fprintf(stderr, "client: you are not connected\n");
		return -1;
	}

	/* Build the packet */
	memset(&packet, 0, sizeof(struct Packet)); // make sure the packet is clean
	packet.action = EXIT;
	strcpy(packet.alias, myalias);

	/* Send the request to close this connection */
	if (send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
	connected = 0;
}

/* Return a pointer to the begin of the message that the user wants to send
input is the string inserted from the user in standard input.
The method processes string formatted like this: "[COMMAND] [ALIAS] [MESSAGE]"
Returns NULL if an error occours or the string is in a wrong format*/
char *getmsg(char *input) {
	int i = 0;
	/* Skip the COMMAND */
	while(input[i++] != ' ');
	/* Skip the ALIAS */
	while(input[i++] != ' ');
	return &input[i];
}

int main(int argc, char *argv[])
{
	printf("Setting up the client, write \"help\" to see a list of commands\n");
	/* Create the buffer where to store the user's input */
	int inputlen = CMDLEN + ALIASLEN; // set the input buffer length properly
	while(1) {
		/* Read a string from standard input */
		char input[inputlen];
		fgets(input, inputlen, stdin);
		/* remove the newline character */
		size_t ln = strlen(input) - 1;
		if (input[ln] == '\n') {
			input[ln] = '\0';
		}
		/* Create a copy of th input string because the method strtok modify it */
		char inputcpy[inputlen];
		strcpy(inputcpy, input);
		/* Read the first token of the string */
		char command[CMDLEN]; // the first token must be the command
		strcpy(command, strtok(inputcpy, " "));
		printf("DEBUG: the input is %s\n the command is %s\n", input, command);
		/* Close the program */
		if(!strncmp(command, "exit", 4) || !strncmp(command, "quit", 4)) {
			/* Clean up and terminate the program */
			printf("Terminating client...\n");
			close(serversfd); // close the listening socket
			break;
		}
		/* Login to the server, optionally add the desired alias as parameter */
		else if(!strncmp(command, "login", 5)) {
			/* Acquire, if present, the parameter */
			char alias[ALIASLEN];
			strcpy(alias, strtok(NULL, " "));
			if(alias != NULL) {
				/* Make sure that the alias is a proper string */
				alias[ALIASLEN] = '\0';
				/* Clean the current alias and set the new one */
				memset(myalias, 0, sizeof(char) * ALIASLEN);
				strcpy(myalias, alias);
				login(myalias);
			}
			else {
				login(NULL); // If there isn't a parameter, login with the default
			}
		}
		else if(!strncmp(command, "alias", 5)) {
			/* Acquire the parameter */
			char alias[ALIASLEN];
			strcpy(alias, strtok(NULL, " "));
			if(alias != NULL) {
				/* Make sure that the alias is a proper string */
				alias[ALIASLEN] = '\0';
				/* Clean the current alias and set the new one */
				memset(myalias, 0, sizeof(char) * ALIASLEN);
				strcpy(myalias, alias);
				setalias(myalias);
			}
			else {
				fprintf(stderr, "Usage: \"alias [NEWALIAS]\"\n");
			}
		}
		else if(!strncmp(input, "whisp", 5)) {
			/* Acquire the first parameter */
			char alias[ALIASLEN];
			strcpy(alias, strtok(NULL, " "));
			int aliaslen =  strlen(alias);
			/* Create a string containing just the message */
			char msg[PAYLEN];
			strcpy(msg, getmsg(input));
			printf("DEBUG: the message is \"%s\"\n", msg);
			if(alias != NULL && msg != NULL) {
				/* Make sure that the alias is a proper string */
				alias[ALIASLEN] = '\0';
				/* Send the message */
				send_msg(alias, msg);
			}
			else {
				fprintf(stderr, "Wrong format.\nUsage: \"whisp [RECIPIENT] [MESSAGE]\"\n");
			}
		}
		else if(!strcmp(command, "logout")) {
			logout();
		}
		else {
			fprintf(stderr, "Unknown command: %s\n", command);
		}
	}
	return 0;
}
