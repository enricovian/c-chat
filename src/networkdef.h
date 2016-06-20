/* Server informations */
#define SERVERIP "localhost"		// the server's address
#define SERVERPORT "3490"  			// port used by the server for incoming connections

#define BACKLOG 8	 				// how many pending connections queue will hold
#define ALIASLEN 32					// maximum length of the client's aliases
#define CMDLEN 32					// maximum length of server's and client's commands
#define DEFAULTALIAS "Anonymous"	// default alias for new clients
#define PAYLEN 2048					// payload size of a single packet, to
									// contain the alias list, it should be at
									// least ALIASLEN*MAXCLIENTS to contain the
									// client's list
#define MAXCLIENTS 64 				// maximum number of clients connected

/* Possible contenents of the packet's "action" field */
#define EXIT 0
#define ALIAS 1
#define MSG 2
#define WHISPER 3
#define SHOUT 4
#define LIST_Q 5	// request to the server to obtain the client list
#define LIST_A 6	// packet containing the client list, is often sent in
					// response to LIST_Q
#define UNF 7		// User Not Found, error packet

/* Structure used by the server program containing the informations regarding a single connection with a client */
struct ClientInfo {
	pthread_t thread_ID; // thread's pointer
	int sockfd; // socket file descriptor
	char alias[ALIASLEN]; // client's alias
};

/* Single packet */
struct Packet {
	unsigned char action; // type of the packet
	char alias[ALIASLEN]; // client's alias
	int len; // length of the payload, necessary only in some packets
	char payload[PAYLEN]; // payload
};

/* get sockaddr correctly formatted: IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
