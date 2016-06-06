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

#include "networkdef.h"

#define PROGRAMNAME client // the name of the program

#define MAXDATASIZE 100 // length in bytes of the buffer where the message are received

int main(int argc, char *argv[])
{
	int sockfd;
    struct addrinfo hints;		// structure used to set the preferencies for servinfo
	struct addrinfo *servinfo;	// structure containing the server's data
    int rv;

	// correct usage checking
    if (argc != 2) {
        fprintf(stderr,"usage: PROGRAMNAME hostname\n");
        return(-1);
    }

	/******************************
	 * Fill up host address
	 ******************************/

	memset(&hints, 0, sizeof hints);	// make sure the struct is empty
    hints.ai_family = AF_UNSPEC;		// use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;	// use TCP protocol for data transmission

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

	/******************************
	 * Connect to the server
	 ******************************/

    // loop through all the results and connect to the first we can
	struct addrinfo *p;	// pointer used to inspect servinfo
    for(p = servinfo; p != NULL; p = p->ai_next) {
		// create the socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;	// in case of error with this address, try with another iteraction
        }

		// attempt to connect
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;	// in case of error with this address, try with another iteraction
        }

        break;	// when there are no more addresses, or one has been successful, exit
    }

    freeaddrinfo(servinfo);

	// if the connection hasn't been successfully established, print an error and exit
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

	// convert the server address to a printable format, then print a message
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

	/******************************
	 * Receive a message
	 ******************************/

	int numbytes;			// length in bytes of the received message
	char buf[MAXDATASIZE];	// buffer where to receive the message

	// wait till a message from the server is received
	switch(numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) {
		// connection interrupted
		case 0 :
			fprintf(stderr, "client: connection interrupted\n");
			return -1;
			break;
		// error case
		case -1 :
			perror("recv");
        	return -1;
			break;
	}

	// make sure the buffer contains a properly formatted string and print the message received
    buf[numbytes] = '\0';
    printf("client: received '%s'\n",buf);

    close(sockfd);

    return 0;
}
