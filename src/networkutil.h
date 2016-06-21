/* Networking libraries */
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* get sockaddr correctly formatted: IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa);
