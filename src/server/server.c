/**
 * @file server.c
 * @brief Server chat application for the c-chat project.
 *
 * @author Enrico Vianello (<enrico.vianello.1@studenti.unipd.it>)
 * @version 1.0
 * @since 1.0
 *
 * @copyright Copyright (c) 2016-2017, Enrico Vianello
 */

#include "server.h"

/* Utility methods to handle network objects */
#include "networkutil.h"

/* Implementation of a list containing the client's informations */
#include "clientlist.h"

/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

/* Networking libraries */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/* Thread library */
#include <pthread.h>

/**
 * Socket listening for incoming connections.
 */
static int sockfd;
/**
 * Socket to perform actions.
 */
int connectfd;
/**
 * Structure used to set the preferencies for servinfo.
 */
struct addrinfo hints;
/**
 * Structure containing local data.
 */
struct addrinfo *servinfo;
/**
 * List of the connected clients.
 */
struct LinkedList client_list;
/**
 * Mutual exclusion variable preventing concurrent edits to the client list.
 */
pthread_mutex_t clientlist_mutex;

/**
 * @brief Display the available commands.
 *
 * @return \c 0 if successful, \c -1 if an error occurred.
 */
int displayhelp();

/**
 * @brief Routine that listens for server's commands.
 *
 * @param param Pointer to a structure containing execution parameters
 * (currently unused, it can be safely set as \c NULL pointer).
 *
 * @return Always a \c NULL pointer.
 */
void *server_handler(void *param);

/**
 * @brief Routine that handles the connection with a client.
 *
 * @param info Pointer to the \c ClientInfo structure relative to the connection
 * that has to be handled.
 *
 * @return Always a \c NULL pointer.
 */
void *client_handler(void *info);

int main(int argc, char *argv[]) {
	/* initialize client list */
	list_init(&client_list);
	/* initiate mutex */
	pthread_mutex_init(&clientlist_mutex, NULL);

	/* initiate thread for server controlling */
	printf("Starting admin interface...\n");
	pthread_t control;
	if(pthread_create(&control, NULL, server_handler, NULL) != 0) {
		perror("server: interface creation");
		return -1;
	}

	/******************************
	 * Set up the listener socket *
	 ******************************/

	memset(&hints, 0, sizeof hints);	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		// use either IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// use TCP protocol for data
										// transmission
	hints.ai_flags = AI_PASSIVE;		// use local IP address

	int status;
	if ((status = getaddrinfo(NULL, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	/* loop through all the elements returned by getaddrinfo and bind the first
	possible */
	struct addrinfo *p;	// pointer used to inspect servinfo
	for(p = servinfo; p != NULL; p = p->ai_next) {
		/* create the socket */
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
			== -1) {
			perror("server: socket");
			/* in case of error with this address, try with another
			iteraction */
			continue;
		}
		/* allow other sockets to bind this port when not listening */
		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
		 	== -1) {
			perror("server: setsockopt");
			return -1;
		}
		/* bind the socket to the address */
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			/* in case of error with this address, try with another
			iteraction */
			continue;
		}
		/* when there are no more addresses or one has been successful, exit */
		break;
	}
	freeaddrinfo(servinfo);

	/* if the socket hasn't been successfully binded print an error and exit */
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}

	/* start listening */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("server: listen");
		return -1;
	}
	printf("Waiting for connections...\n");

	/************************
	 * Connections handling *
	 ************************/
	/* connector's address information */
	struct sockaddr_storage client_addr;
	/* dimension of the connector's sockaddr structure */
	socklen_t sin_size;
	/* temporary file descriptor for the incoming connections */
	int new_fd = -1;

	while(1) {  // main accept() loop
		/* block the server till a pending connection request is present,
		then accept it */
		sin_size = sizeof client_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);

		/* if an error occours, print an error and go on with the next
		iteration */
		if (new_fd == -1) {
			perror("server: accept");
			continue;
		}

		/* convert the client address to a printable format, then print a
		message */
		char s[INET6_ADDRSTRLEN];
		inet_ntop(client_addr.ss_family,
			get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
		printf("Got connection from %s\n", s);

		/* Set the client data */
		struct  ClientInfo client_info;
		client_info.sockfd = new_fd;
		strcpy(client_info.alias, DEFAULTALIAS);

		/* Add the new client to the client list */
		pthread_mutex_lock(&clientlist_mutex);
		list_insert(&client_list, &client_info);
		pthread_mutex_unlock(&clientlist_mutex);

		/* Create a thread to handle the new client */
		pthread_create(
			&client_info.thread_ID,
			NULL,
			client_handler,
			(void *)&client_info
		);
	}

	return 0;
}

/**
* @brief Display the available commands.
*
* @return \c 0 if successful, \c -1 if an error occurred.
*/
int displayhelp() {
	char c;
	FILE *file;
	file = fopen("../helpfiles/server_help.txt", "r");
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

/**
 * @brief Routine that listens for server's commands.
 *
 * @param param Pointer to a structure containing execution parameters
 * (currently unused, it can be safely set as \c NULL pointer).
 *
 * @return Always a \c NULL pointer.
 */
void *server_handler(void *param) {
	char command[CMDLEN];
	while(scanf("%s", command) == 1) {
		/* Clean up and terminate the program */
		if(!strcmp(command, "/exit") || !strcmp(command, "/quit")) {
			printf("Terminating server...\n");
			pthread_mutex_destroy(&clientlist_mutex); // delete the mutex
			close(sockfd); // close the listening socket
			exit(0);
		}
		/* Print a dump of the current client list */
		else if(!strcmp(command, "/list")) {
			pthread_mutex_lock(&clientlist_mutex);
			list_dump(&client_list);
			pthread_mutex_unlock(&clientlist_mutex);
		}
		/* Print an help text */
		else if(!strcmp(command, "/help")) {
			displayhelp();
		}
		else {
			fprintf(stderr, "Unknown command: %s\n", command);
		}
	}
	return NULL;
}

/**
 * @brief Routine that handles the connection with a client.
 *
 * @param info Pointer to the \c ClientInfo structure relative to the connection
 * that has to be handled.
 *
 * @return Always a \c NULL pointer.
 */
void *client_handler(void *info) {
	struct ClientInfo client_info = *(struct ClientInfo *)info;
	struct Packet packet;
	struct LLNode *curr;
	while(1) {
		/* Receive a packet of data from the client */
		if(!recv(client_info.sockfd, (void *)&packet, sizeof(struct Packet),
			0)) {
			/* Connection with the client lost */
			fprintf(stderr, "Connection lost from [%d] %s\n",
				client_info.sockfd, client_info.alias);
			/* Remove the client from the client list */
			pthread_mutex_lock(&clientlist_mutex);
			/* remove the client from the linked list */
			list_delete(&client_list, &client_info);
			pthread_mutex_unlock(&clientlist_mutex);
			break;
		}
		printf("Packet received:[%d] action_code=%d | %s | %s\n",
			client_info.sockfd, packet.action, packet.alias, packet.payload);
		switch (packet.action) {
			/* Change the client's alias */
			case ALIAS :
				printf("User #%d is changing his alias from '%s' to '%s'\n",
					client_info.sockfd, client_info.alias, packet.alias);
				pthread_mutex_lock(&clientlist_mutex);
				/* Search the client in the list and edit his alias */
				for(curr = client_list.head; curr != NULL; curr = curr->next) {
					if(compare(&curr->client_info, &client_info) == 0) {
						strcpy(curr->client_info.alias, packet.alias);
						strcpy(client_info.alias, packet.alias);
					}
				}
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			/* Send a message to a specific client */
			case WHISPER : ; // empty statement necessary to compile
				/* Acquire the target client */
				char target[ALIASLEN];
				int i;
				for(i = 0; packet.payload[i] != ' '; i++);
				/* replace the space after the target's alias with a
				termination */
				packet.payload[i++] = '\0';
				strcpy(target, packet.payload);
				/* Find the target client and send the message */
				int found = 0; // 1 if the client has been found
				pthread_mutex_lock(&clientlist_mutex);
				for(curr = client_list.head; curr != NULL; curr = curr->next) {
					if(strcmp(target, curr->client_info.alias) == 0) {
						/* If the found client is the sender, keep searching */
						if(!compare(&curr->client_info, &client_info)) {
							continue;
						}
						found = 1;
						/* Build a new packet only containing the message */
						struct Packet msgpacket;
						memset(&msgpacket, 0, sizeof(struct Packet));
						msgpacket.action = MSG;
						strcpy(msgpacket.alias, packet.alias);
						/* the payload of the new packet contains just the
						message */
						strcpy(msgpacket.payload, &packet.payload[i]);
						if (send(curr->client_info.sockfd, (void *)&msgpacket,
							sizeof(struct Packet), 0) == -1) {
							perror("server: send");
						}
					}
				}
				pthread_mutex_unlock(&clientlist_mutex);
				/* If the specified user has not been found, send back to the
				client an UNF (User Not Found) packet */
				if (!found) {
					struct Packet errpacket;
					memset(&errpacket, 0, sizeof(struct Packet));
					errpacket.action = UNF;
					/* The alias field contains the client not found */
					strcpy(errpacket.alias, target);
					if (send(client_info.sockfd, (void *)&errpacket,
						sizeof(struct Packet), 0) == -1) {
						perror("server: send");
					}
				}
				break;
			/* Send a message to every client connected */
			case SHOUT :
				pthread_mutex_lock(&clientlist_mutex);
				for(curr = client_list.head; curr != NULL; curr = curr->next) {
					/* If the found client is the sender, keep searching */
					if(!compare(&curr->client_info, &client_info)) {
						continue;
					}
					/* Build a new packet containing the message and send it */
					struct Packet msgpacket;
					memset(&msgpacket, 0, sizeof(struct Packet));
					msgpacket.action = MSG;
					strcpy(msgpacket.alias, packet.alias);
					strcpy(msgpacket.payload, packet.payload);
					if (send(curr->client_info.sockfd, (void *)&msgpacket,
						sizeof(struct Packet), 0) == -1) {
						perror("server: send");
					}
				}
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			/* Client's list request */
			case LIST_Q :
				pthread_mutex_lock(&clientlist_mutex);
				int size = list_size(&client_list);
				char **list_str;
				list_str = list_clients(&client_list);
				/* Build a new packet containing the list */
				struct Packet answer_packet;
				memset(&answer_packet, 0, sizeof(struct Packet));
				answer_packet.action = LIST_A;
				answer_packet.len = size; // number of clients in the list
				strcpy(answer_packet.alias, packet.alias);
				/* Insert the client's aliases in the packet's payload */
				for(int i = 0; i < size; i++) {
					memcpy(&answer_packet.payload[ALIASLEN*i], list_str[i],
						ALIASLEN*sizeof(char));
				}
				/* Send the packet */
				if (send(client_info.sockfd, (void *)&answer_packet,
					sizeof(struct Packet), 0) == -1) {
					perror("server: send");
				}
				free(list_str); // deallocate the memory used for the list
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			/* Terminate the connection */
			case EXIT :
				printf("[%d] %s has disconnected\n", client_info.sockfd,
					client_info.alias);
				pthread_mutex_lock(&clientlist_mutex);
				list_delete(&client_list, &client_info);
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			default :
				fprintf(stderr,
					"Unidentified packet from [%d] %s : action_code=%d\n",
					client_info.sockfd, client_info.alias, packet.action);
		}
	}

	/* Close the client socket */
	close(client_info.sockfd);

	return NULL;
}
