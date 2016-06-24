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

/* The server's address */
#define SERVERIP "localhost"
/* Port used by the server for incoming connections */
#define SERVERPORT "3490"
/* How many pending connections queue will hold */
#define BACKLOG 8
