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

/* get sockaddr correctly formatted: IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
