/**
 * @file networkutil.h
 * @brief Collection of utility methods used in the c-chat project.
 *
 * @author Enrico Vianello (<enrico.vianello.1@studenti.unipd.it>)
 * @version 1.0
 * @since 1.0
 *
 * @copyright Copyright (c) 2016-2017, Enrico Vianello
 */

/* Networking libraries */
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
* @brief Get the address structure correctly formatted: IPv4 or IPv6 from a
* generic \c sockaddr structure.
*
* @param sa The \c sockaddr struct.
*
* @return A pointer to a structure \c sin_addr (if the address is
* IPv4) or \c sin6_addr (if the address is IPv6).
*/
void *get_in_addr(struct sockaddr *sa);
