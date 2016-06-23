/**
 * @file networkdef.h
 * @brief Parameters, structures and definitions relative to the applications'
 * behaviour and the connection's protocol.
 *
 * @author Enrico Vianello (<enrico.vianello.1@studenti.unipd.it>)
 * @version 1.0
 * @since 1.0
 *
 * @copyright Copyright (c) 2016-2017, Enrico Vianello
 */

/* Necessary for the definition of pthread_t */
#include <pthread.h>

/*************************
 * Connection parameters *
 *************************/

/* Maximum length of the client's aliases */
#define ALIASLEN 32
/* Maximum length of server's and client's commands */
#define CMDLEN 32
/* Default alias for new clients */
#define DEFAULTALIAS "Anonymous"
/* Payload size of a single packet.
To contain the alias list, it should be at least ALIASLEN*MAXCLIENTS */
#define PAYLEN 2048
/* maximum number of clients connected */
#define MAXCLIENTS 64

/******************************************************
 * Possible contenents of the packet's "action" field *
 ******************************************************/

#define EXIT 0		// disconnection request
#define ALIAS 1		// alias changing request
#define MSG 2		// message packet
#define WHISPER 3	// request to contact a specified client
#define SHOUT 4		// request to send a broadcast message
#define LIST_Q 5	// request to the server to obtain the client list
#define LIST_A 6	// packet containing the client list, is often sent in
					// response to LIST_Q
#define UNF 7		// User Not Found, error packet

/*************************
 * Structure definitions *
 *************************/

/**
 * @struct ClientInfo
 *
 * @brief Structure containing the informations regarding a single connection
 * with a client.
 *
 * This structure is used by the server application
 *
 * @var ClientInfo::thread_ID Pointer to the server's thread associated to this
 * connection.
 * @var ClientInfo::sockfd Socket file descriptor associated with this
 * connection.
 * @var ClientInfo::alias Alias of the client associated to this connection.
 */
struct ClientInfo {
	pthread_t thread_ID;
	int sockfd;
	char alias[ALIASLEN];
};

/**
 * @struct Packet
 *
 * @brief Structure representing a single packet exchanged between the server
 * and the clients.
 *
 * @var Packet::action Action code of this packet. The possible values can
 * be found in the definitions of the file \c networkdef.h
 * @var ClientInfo::alias Field that can contain an alias. Its use is explained
 * in the protocol description.
 * @var ClientInfo::len Field that can contain an integer. Its use is explained
 * in the protocol description.
 * @var ClientInfo::payload Packet's main body.
 */
struct Packet {
	unsigned char action;
	char alias[ALIASLEN];
	int len;
	char payload[PAYLEN];
};
