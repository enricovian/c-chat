#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
	int sockfd;								// socket listening for incoming connections
	int connectfd;							// socket to perform actions
	struct addrinfo hints;					// structure used to set the preferencies for servinfo
	struct addrinfo *servinfo;				// structure containing local data
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];


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
		perror("listen");
		exit(1);
	}
	printf("server: waiting for connections...\n");

	/******************************
	 * Main while loop
	 ******************************/

	struct sockaddr_storage client_addr; 	// connector's address information
	socklen_t sin_size;						// dimension of the connector's sockaddr structure
	int new_fd = -1;						// temporary file descriptor for the incoming connections

	while(1) {  // main accept() loop
		// block the server till a pending connection request is present, then accept it
		sin_size = sizeof client_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);


		// if an error occours, print an error and go on with the next iteration
		if (new_fd == -1) {
			perror("accept");
			printf("DEBUG: no client found\n");
			continue;
		}

		printf("DEBUG: client found!\n");

		// convert the client address to a printable format, then print a message
		char s[INET6_ADDRSTRLEN];
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		// fork the current process making a child process to handle the connection (see fork() documentation)
		switch(fork()) {
			// continuation for the child process
			case 0 :
				close(sockfd); // the child process doesn't need the listener socket
				// send a simple string
				if (send(new_fd, "Hello, world!", 13, 0) == -1) {
					perror("send");
				}
				close(new_fd); // close the connection
				return 0;
				break;
			// error case
			case -1 :
				perror("fork");
				break;
			// continuation for the parent process
			default :
				close(new_fd);  // the parent process doesn't need the connection socket
		}


	}

	return 0;
}
