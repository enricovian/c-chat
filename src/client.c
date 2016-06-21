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

/* Definitions about connection and protocol parameters */
#include "networkdef.h"

/* Utility methods to handle network objects */
#include "networkutil.h"

/* Connection status: 1 if connected with a server, 0 elsewhere */
int connected;
/* Socket file descriptor of the connection with the server */
int serversfd;
/* Alias of the client */
char myalias[ALIASLEN];
/* Server's IP */
char server_ip[INET6_ADDRSTRLEN];
/* Server's port */
char server_port[6];

/* Routine that constantly listens for incoming packets.
The method returns a void pointer because it is needed to create a thread */
static void *receiver();

/* Establish a connection with the server.
Return -1 if an error occurred, the socket file descriptor elsewhere */
static int connect_server(char *ip, char *port);

/* Send a request to the server to change your own alias to name */
static int setalias(char name[]);

/* Login to the server, if name is not NULL, the alias is set accordingly */
static int login(char *ip, char *port, char *name);

/* Send a message to a client specifying his alias */
static int send_msg(char target[], char msg[]);

/* Send a message to every client connected */
static int broadcast_msg(char msg[]);

/* Interrupt the connection with the server */
static int askforlist();

/* Interrupt the connection with the server */
static int logout();

/* Display a text file containing the possible commands */
static int displayhelp();

/* Return a pointer to the begin of the message that the user wants to send
input is the string inserted from the user in standard input.
The method processes string formatted like this: "[COMMAND] [ALIAS] [MESSAGE]"
Returns NULL if an error occours or the string is in a wrong format*/
static char *getmsg(char *input);

/* Remove extra character from the buffer of input, returns the last character
read */
static char flushinput(FILE *input);

int main(int argc, char *argv[])
{
	printf("Setting up the client, write \"help\" to see a list of commands\n");
	/* Create the buffer where to store the user's input */
	int buflen = 64; // PAYLEN; // set the input buffer length properly
	while(1) {
		/* Read a string from standard input */
		char input[buflen];
		fgets(input, buflen, stdin);
		/* If the last char gathered is the newline character remove it,
		elsewhere the input is too long and the input buffer must be cleaned at
		the end of the cycle */
		size_t inputlen = strlen(input) - 1;
		int buf_overflow = 0;
		if (input[inputlen] == '\n') {
			input[inputlen] = '\0';
		} else {
			buf_overflow = 1;
		}
		/* Check if the inserted string is a command or a chat message */
		if (input[0] != '/') {
			/* Send a chat message to every client connected */
			broadcast_msg(input);
		} else {
			/* Create a copy of the input string because the method strtok modifies it */
			char inputcpy[buflen];
			strcpy(inputcpy, input);
			/* Read the first token of the string */
			char command[CMDLEN]; // the first token must be the command
			strcpy(command, strtok(inputcpy, " "));
			/* Close the program */
			if(!strncmp(command, "/exit", 5) || !strncmp(command, "/quit", 5)) {
				/* Clean up and terminate the program */
				printf("Terminating client...\n");
				close(serversfd); // close the listening socket
				break;
			}
			/* Login to the server specifying IP and port, optionally add the
			desired alias as parameter */
			else if(!strncmp(command, "/login", 6)) {
				/* Acquire the first parameter: the server's address */
				char *server_ip = strtok(NULL, " ");
				/* Acquire the second parameter */
				char *server_port = strtok(NULL, " ");
				/* Acquire, if present, the parameter */
				char *alias = strtok(NULL, " ");
				if (server_ip != NULL && server_port != NULL) {
					if(alias != NULL) {
						/* If the alias is too long, truncate it */
						alias[ALIASLEN] = '\0';
						/* Clean the current alias and set the new one */
						memset(myalias, 0, sizeof(char) * ALIASLEN);
						strcpy(myalias, alias);
						login(server_ip, server_port, myalias);
					}
					else {
						login(server_ip, server_port, NULL); // If there isn't a parameter, login with the default alias
					}
				} else {
					fprintf(stderr, "Usage: \"/login [SERVER_IP] [SERVER_PORT] [ALIAS]\"\n");
				}
			}
			/* Change the alias */
			else if(!strncmp(command, "/alias", 6)) {
				/* Acquire the parameter */
				char *alias = strtok(NULL, " ");
				if(alias != NULL) {
					/* If the alias is too long, truncate it */
					alias[ALIASLEN] = '\0';
					/* Clean the current alias and set the new one */
					memset(myalias, 0, sizeof(char) * ALIASLEN);
					strcpy(myalias, alias);
					setalias(myalias);
				}
				else {
					fprintf(stderr, "Usage: \"/alias [NEWALIAS]\"\n");
				}
			}
			/* Send a message to a specific client */
			else if(!strncmp(input, "/whisp", 6)) {
				/* Acquire the first parameter */
				char *alias = strtok(NULL, " ");
				/* Create a string containing just the message */
				char msg[PAYLEN];
				strcpy(msg, getmsg(input));
				if(alias != NULL && msg != NULL) {
					/* Make sure that the alias is a proper string */
					alias[ALIASLEN-1] = '\0';
					/* Send the message */
					send_msg(alias, msg);
				}
				else {
					fprintf(stderr, "Usage: \"/whisp [RECIPIENT] [MESSAGE]\"\n");
				}
			}
			/* List the clients currently connected */
			else if(!strncmp(input, "/list", 5)) {
				askforlist();
			}
			/* Terminate the connection */
			else if(!strcmp(command, "/logout")) {
				logout();
			}
			/* Print an help text */
			else if(!strcmp(command, "/help")) {
				displayhelp();
			}
			else {
				fprintf(stderr, "Unknown command: %s\n", command);
			}
		}
		/* If there are extra characters in the input's buffer, remove them */
		if (buf_overflow) {
			flushinput(stdin);
		}
	}
	return 0;
}

/* Routine that constantly listens for incoming packets.
The method returns a void pointer because it is needed to create a thread */
static void *receiver() {
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
		switch (packet.action) {
			/* Message to display received */
			case MSG :
				/* Display the message on screen. TODO: create a dedicate method */
				printf("[%s]: %s\n", packet.alias, packet.payload);
				break;
			/* List of clients received */
			case LIST_A : ;
				/* Display the clients connected to the server */
				printf("There are %d clients connected:\n", packet.len);
				for(int i = 0; i < packet.len; i++) {
					printf("[%d] %s\n", i+1, &packet.payload[ALIASLEN*i]);
				}
				break;
			/* There are no clients with the alias specifie in the whisper command */
			case UNF :
				printf("Client \"%s\" not found. Type /list to see the clients connected\n", packet.alias);
				break;
		}

		/* Clean the packet */
		memset(&packet, 0, sizeof(struct Packet));
	}
	return NULL;
}

/* Establish a connection with the server.
Return -1 if an error occurred, the socket file descriptor elsewhere */
static int connect_server(char *ip, char *port) {
	/* Prepare the server's address */
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	if ((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
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
		return newfd;
	}
	freeaddrinfo(servinfo); // free the linked-list
	freeaddrinfo(&hints);
	return 0;
}

/* Send a request to the server to change your own alias to name */
static int setalias(char name[]) {
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
static int login(char *ip, char *port, char *name) {
	if (connected) {
		fprintf(stderr, "client: you are already connected to server at %s:%s\n", server_ip, server_port);
		return -1;
	}
	/* Temporary variable containing the socket file declarator of the connection */
	int sockfd;
	if ((sockfd = connect_server(ip, port)) == -1) {
		fprintf(stderr, "client: error occurred in method connect_server\nclient: connection failed\n");
		return -1;
	}
	if(sockfd >= 0) {
		connected = 1;
		serversfd = sockfd;
		if(name != NULL) {
			setalias(name);
		} else {
			strcpy(myalias, DEFAULTALIAS);
		}
		/* Start a thread to handle the packages' reception */
		pthread_t receive;
		pthread_create(&receive, NULL, receiver, NULL);
	}
	else {
		fprintf(stderr, "client: connection failed\n");
		return -1;
	}
	strcpy(server_ip, ip);
	strcpy(server_port, port);
	printf("client: connected to server at %s:%s as %s\n", server_ip, server_port, myalias);
	return 0;
}

/* Send a message to a client specifying his alias */
static int send_msg(char target[], char msg[]) {
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
	printf("DEBUG: the packet's payload is \"%s\"\n", packet.payload);
	/* Send the packet */
	if (send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
	return 0;
}

/* Send a message to every client connected */
static int broadcast_msg(char msg[]) {
	struct Packet packet;
	if(msg == NULL) {
		return 0;
	}
	if(!connected) {
		fprintf(stderr, "client: you are not connected\n");
		return -1;
	}
	/* Build the packet */
	memset(&packet, 0, sizeof(struct Packet)); // make sure the packet is clean
	packet.action = SHOUT;
	strcpy(packet.alias, myalias);
	/* In the packet's payload insert the message */
	strcpy(packet.payload, msg);
	/* Send the packet */
	if (send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
	return 0;
}

/* Interrupt the connection with the server */
static int askforlist() {
	struct Packet packet;

	if(!connected) {
		fprintf(stderr, "client: you are not connected\n");
		return -1;
	}

	/* Build the packet */
	memset(&packet, 0, sizeof(struct Packet)); // make sure the packet is clean
	packet.action = LIST_Q;
	strcpy(packet.alias, myalias);
	if (send(serversfd, (void *)&packet, sizeof(struct Packet), 0) == -1) {
		perror("client: send");
		return -1;
	}
}

/* Interrupt the connection with the server */
static int logout() {
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
	server_ip[0] = '\0';
	server_port[0] = '\0';
}

/* Display a text file containing the possible commands */
static int displayhelp() {
	char c;
	FILE *file;
	file = fopen("../helpfiles/client_help.txt", "r");
	if (file == NULL) {
		perror("client: fopen, error opening help file");
		return(-1);
	}
	while ((c = getc(file)) != EOF) {
		putchar(c);
	}
	fclose(file);
	return 0;
}

/* Return a pointer to the begin of the message that the user wants to send
input is the string inserted from the user in standard input.
The method processes string formatted like this: "[COMMAND] [ALIAS] [MESSAGE]"
Returns NULL if an error occours or the string is in a wrong format*/
static char *getmsg(char *input) {
	int i = 0;
	/* Skip the COMMAND */
	while(input[i++] != ' ');
	/* Skip the ALIAS */
	while(input[i++] != ' ');
	return &input[i];
}

/* Remove extra character from the buffer of input, returns the last character
read */
static char flushinput(FILE *input) {
	char ch;
	while (((ch = fgetc(input)) != '\n') && (ch != EOF)) ;
	return ch;
}
