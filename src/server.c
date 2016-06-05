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
#define MAXCLIENTS 64 // maximum number of clients connected

/* Possible contenents of the packet's "action" field */
#define EXIT 0
#define ALIAS 1
#define MSG 2
#define WHISPER 3
#define SHOUT 4

/******************************
* STRUCTURE DEFINITION
 ******************************/

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

/******************************
 * END OF STRUCTURE DEFINITION
 ******************************/

/******************************
 * LINKED LIST DEFINITION
 * Every node of the list contains informations about one client
 ******************************/

/* Node of the list */
struct LLNode {
	struct ClientInfo client_info;
	struct LLNode *next;
};

/* List */
struct LinkedList {
	struct LLNode *head, *tail;
	int size;
};

/* Compare two ClientInfo struct, return 0 if they have the same connection socket */
int compare(struct ClientInfo *a, struct ClientInfo *b) {
	return a->sockfd - b->sockfd;
}

/* Initialize an empty list */
void list_init(struct LinkedList *ll) {
	ll->head = ll->tail = NULL;
	ll->size = 0;
}

/* Insert an element to the list, return -1 if the list is full */
int list_insert(struct LinkedList *ll, struct ClientInfo *cl_info) {
	if(ll->size == MAXCLIENTS) return -1; // check if the list is full
	/* If the list is empty, create the node and make head and tail point to it */
	if(ll->head == NULL) {
		ll->head = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->head->client_info = *cl_info;
		ll->head->next = NULL;
		ll->tail = ll->head;
	}
	/* If the list isn't empty, create a new node and make the tail point to it */
	else {
		ll->tail->next = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->tail->next->client_info = *cl_info;
		ll->tail->next->next = NULL;
		ll->tail = ll->tail->next;
	}
	ll->size++;
	return 0;
}

/* Delete a client node from the list, return -1 if the element isn't found */
int list_delete(struct LinkedList *ll, struct ClientInfo *cl_info) {
	struct LLNode *curr, *tmp;
	if(ll->head == NULL) return -1; // check if the structure is empty
	/* Handle the case where the element to remove is the first */
	if(!compare(cl_info, &ll->head->client_info)) {
		tmp = ll->head; // store the element in a temporary variable to free the memory
		ll->head = ll->head->next;
		/* If the list is empty after the deletion, handle it correctly */
		if(ll->head == NULL) {
			ll->tail = ll->head;
		}
		free(tmp);
		ll->size--;
		return 0;
	}
	/* Search for the element to remove through the list */
	for(curr = ll->head; curr->next != NULL; curr = curr->next) {
		if(!compare(cl_info, &curr->next->client_info)) {
			tmp = curr->next; // store the element in a temporary variable to free the memory
			/* Handle the case where the element to remove is the last */
			if(tmp == ll->tail) {
				ll->tail = curr;
				curr->next = NULL;
			} else {
				curr->next = curr->next->next;
			}
			free(tmp);
			ll->size--;
			return 0;
		}
	}
	return -1;
}

/* Print the clients in a readable format */
void list_dump(struct LinkedList *ll) {
	struct LLNode *curr;
	struct ClientInfo *cl_info;
	printf("Connection count: %d\n", ll->size);
	for(curr = ll->head; curr != NULL; curr = curr->next) {
		cl_info = &curr->client_info;
		printf("[%d] %s\n", cl_info->sockfd, cl_info->alias);
	}
}

/******************************
 * END OF LINKED LIST
 ******************************/

/* FIELDS */
static int sockfd;					// socket listening for incoming connections
int connectfd;						// socket to perform actions
struct addrinfo hints;				// structure used to set the preferencies for servinfo
struct addrinfo *servinfo;			// structure containing local data
struct LinkedList client_list;		// list of the connected clients
pthread_mutex_t clientlist_mutex;	// mutual exclusion variable preventing concurrent
									// edits to the client list

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
			/* Clean up and terminate the program */
			printf("Terminating server...\n");
			pthread_mutex_destroy(&clientlist_mutex); // delete the mutex
			close(sockfd); // close the listening socket
			exit(0);
		}
		else if(!strcmp(command, "list")) {
			/* Print a dump of the current client list */
			pthread_mutex_lock(&clientlist_mutex);
			list_dump(&client_list);
			pthread_mutex_unlock(&clientlist_mutex);
		}
		else {
			fprintf(stderr, "Unknown command: %s\n", command);
		}
	}
	return NULL;
}

void *client_handler(void *info) {
	struct ClientInfo client_info = *(struct ClientInfo *)info;
	struct Packet packet;
	struct LLNode *curr;
	printf("DEBUG: Thread started for the client named %s\n", client_info.alias);
	while(1) {
		/* Receive a packet of data from the client */
		if(!recv(client_info.sockfd, (void *)&packet, sizeof(struct Packet), 0)) {
			/* Connection with the client lost */
			fprintf(stderr, "Connection lost from [%d] %s\n", client_info.sockfd, client_info.alias);
			/* Remove the client from the client list */
			pthread_mutex_lock(&clientlist_mutex);
			list_delete(&client_list, &client_info); // remove the client from the linked list
			pthread_mutex_unlock(&clientlist_mutex);
			break;
			printf("DEBUG: This shouldn't be printed if a client thread is closed");
		}
		printf("Packet received:[%d] action_code=%d | %s | %s\n", client_info.sockfd, packet.action, packet.alias, packet.payload);
		switch (packet.action) {
			/* Change the client's alias */
			case ALIAS :
				printf("server: user #%d is changing his alias from '%s' to '%s'\n", client_info.sockfd, client_info.alias, packet.alias);
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
				int i;
				char target[ALIASLEN];
				for(i = 0; packet.payload[i] != ' '; i++);
				packet.payload[i++] = '\0'; // replace the space after the target's alias with a termination
				strcpy(target, packet.payload);
				/* Find the target client and send the message */
				pthread_mutex_lock(&clientlist_mutex);
				for(curr = client_list.head; curr != NULL; curr = curr->next) {
					if(strcmp(target, curr->client_info.alias) == 0) {
						/* If the found client is the sender, keep searching */
						if(!compare(&curr->client_info, &client_info)) {
							continue;
						}
						/* Build a new packet only containing the message */
						struct Packet msgpacket;
						memset(&msgpacket, 0, sizeof(struct Packet));
						msgpacket.action = MSG;
						strcpy(msgpacket.alias, packet.alias);
						strcpy(msgpacket.payload, &packet.payload[i]); // the payload of the new packet contains just the message
						if (send(curr->client_info.sockfd, (void *)&msgpacket, sizeof(struct Packet), 0) == -1) {
							perror("server: send");
						}
					}
				}
				pthread_mutex_unlock(&clientlist_mutex);
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
					if (send(curr->client_info.sockfd, (void *)&msgpacket, sizeof(struct Packet), 0) == -1) {
						perror("server: send");
					}
				}
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			/* Terminate the connection */
			case EXIT :
				printf("[%d] %s has disconnected\n", client_info.sockfd, client_info.alias);
				pthread_mutex_lock(&clientlist_mutex);
				list_delete(&client_list, &client_info);
				pthread_mutex_unlock(&clientlist_mutex);
				break;
			default :
				fprintf(stderr, "Unidentified packet from [%d] %s : action_code=%d\n", client_info.sockfd, client_info.alias, packet.action);
		}
	}

	/* Close the client socket */
	close(client_info.sockfd);

	return NULL;
}

int main(void) {
	/* initialize client list */
	list_init(&client_list);
	/* initiate mutex */
	pthread_mutex_init(&clientlist_mutex, NULL);

	/******************************
	 * Set up the listener socket
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

	// loop through all the elements returned by getaddrinfo and bind the first possible
	struct addrinfo *p;	// pointer used to inspect servinfo
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// create the socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			printf("DEBUG: socket creation caused an error\n");
			continue;	// in case of error with this address, try with another iteraction
		}
		// allow other sockets to bind this port when not listening
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

	struct sockaddr_storage client_addr;	// connector's address information
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

		/* Set the client data */
		struct  ClientInfo client_info;
		client_info.sockfd = new_fd;
		strcpy(client_info.alias, DEFAULTALIAS);

		/* Add the new client to the client list */
		pthread_mutex_lock(&clientlist_mutex);
		list_insert(&client_list, &client_info);
		pthread_mutex_unlock(&clientlist_mutex);

		/* Create a thread to handle the new client */
		pthread_create(&client_info.thread_ID, NULL, client_handler, (void *)&client_info);
	}

	return 0;
}
