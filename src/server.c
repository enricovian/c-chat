#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 8	 // how many pending connections queue will hold
#define ALIASLEN 32	// maximum length of the client's aliases
#define CMDLEN 32	// maximum length of the server's commands
#define DEFAULTALIAS "Anonymous"	// default alias for new clients
#define PAYLEN 1024	// payload size of a single packet

/* Possible contenents of the packet's "action" field */
#define EXIT 0
#define ALIAS 1
#define WHISPER 2
#define SEND 3

/* Structure containing the informations regarding a single connection with a client */
struct ClientInfo {
	pthread_t thread_ID; // thread's pointer
	int sockfd; // socket file descriptor
	char alias[ALIASLEN]; // client's alias
};

/* Single packet */
struct Packet {
    unsigned char action; // type of the packet
    char alias[ALIASLEN]; // client's alias
    char payload[PAYLEN]; // payload
};

/* FIELDS */
static int sockfd;									// socket listening for incoming connections
int connectfd;							// socket to perform actions
struct addrinfo hints;					// structure used to set the preferencies for servinfo
struct addrinfo *servinfo;				// structure containing local data

// get sockaddr, correctly formatted: IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *server_handler(void *param) {
	char command[CMDLEN];
	while(scanf("%s", command) == 1) {
		if(!strcmp(command, "exit") || !strcmp(command, "quit")) {
			/* clean up */
			printf("Terminating server...\n");
//			pthread_mutex_destroy(&clientlist_mutex);
			close(sockfd); // close the listening socket
			exit(0);
		}
//		else if(!strcmp(command, "list")) {
//			pthread_mutex_lock(&clientlist_mutex);
//			list_dump(&client_list);
//			pthread_mutex_unlock(&clientlist_mutex);
//		}
		else {
			fprintf(stderr, "Unknown command: %s\n", command);
		}
	}
	return NULL;
}

void *client_handler(void *info) {
	struct ClientInfo client_info = *(struct ClientInfo *)info;
	struct Packet packet;
	struct LLNODE *curr;
	int sent;
	printf("DEBUG: Thread started for the client named %s\n", client_info.alias);
	while(1) {
		/* Receive a packet of data from the client */
		if(!recv(client_info.sockfd, (void *)&packet, sizeof(struct Packet), 0)) {
			/* Connection with the client lost */
			fprintf(stderr, "Connection lost from [%d] %s...\n", client_info.sockfd, client_info.alias);
//			pthread_mutex_lock(&clientlist_mutex);
//			list_delete(&client_list, &client_info); // remove the client from the linked list
//			pthread_mutex_unlock(&clientlist_mutex);
			break;
		}
		/* Print the entire contenent of the received packet */
		printf("[%d] action_code:%d | %s | %s\n", client_info.sockfd, packet.action, packet.alias, packet.payload);
	}

	/*  */
	close(client_info.sockfd);

	return NULL;
}

int main(void) {

	/******************************
	 * Fill up local address
	 ******************************/

	memset(&hints, 0, sizeof hints);	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		// use either IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// use TCP protocol for data transmission
	hints.ai_flags = AI_PASSIVE;		// use local IP address

	int status;
	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	printf("DEBUG: Local address ready\n");

	/******************************
	 * Set the listener
	 ******************************/

	// loop through all the elements returned by getaddrinfo and bind the first possible
	struct addrinfo *p;	// pointer used to inspect servinfo
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// create the socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			printf("DEBUG: socket creation caused an error\n");
			continue;	// in case of error with this address, try with another iteraction
		}
		// allow other socket to bind this port when not listening
		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("server: setsockopt");
			printf("DEBUG: socket setting caused an error\n");
			return -1;
		}
		// bind the socket to the address
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			printf("DEBUG: binding socket to address caused an error\n");
			continue;	// in case of error with this address, try with another iteraction
		}

		break; // when there are no more addresses, or one has been successful, exit
	}

	freeaddrinfo(servinfo);

	// if the socket hasn't been successfully binded, print an error and exit
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}

	printf("DEBUG: socket successfully binded to a local address\n");

	// start listening
	if (listen(sockfd, BACKLOG) == -1) {
		perror("server: listen");
		return -1;
	}
	printf("server: waiting for connections...\n");

	/******************************
	 * Connections handling
	 ******************************/

	/* initiate thread for server controlling */
	printf("Starting admin interface...\n");
	pthread_t control;
	if(pthread_create(&control, NULL, server_handler, NULL) != 0) {
		perror("server: interface creation");
		return -1;
	}

	struct sockaddr_storage client_addr; 	// connector's address information
	socklen_t sin_size;						// dimension of the connector's sockaddr structure
	int new_fd = -1;						// temporary file descriptor for the incoming connections

	while(1) {  // main accept() loop
		// block the server till a pending connection request is present, then accept it
		sin_size = sizeof client_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);

		// if an error occours, print an error and go on with the next iteration
		if (new_fd == -1) {
			perror("server: accept");
			continue;
		}

		// convert the client address to a printable format, then print a message
		char s[INET6_ADDRSTRLEN];
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		/* set the necessary data and start a thread to handle the connection */
		struct  ClientInfo client_info;
		client_info.sockfd = new_fd;
		strcpy(client_info.alias, DEFAULTALIAS);
//		pthread_mutex_lock(&clientlist_mutex);
//		list_insert(&client_list, &client_info);
//		pthread_mutex_unlock(&clientlist_mutex);

		/* create a thread to handle the new client */
		pthread_create(&client_info.thread_ID, NULL, client_handler, (void *)&client_info);

	}

	return 0;
}
