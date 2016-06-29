/**
 * @file clientlist.h
 * @brief Linked list implementation where every node represent a connection
 * with a client.
 *
 * @author Enrico Vianello (<enrico.vianello.1@studenti.unipd.it>)
 * @version 1.0
 * @since 1.0
 *
 * @copyright Copyright (c) 2016-2017, Enrico Vianello
 */

/* Necessary for the definition of the struct ClientInfo */
#include "networkdef.h"

/**
 * @struct LLNode
 *
 * @brief Node of the linked list, representing a connection.
 *
 * @var LLNode::client_info
 * \c ClientInfo struct containing the actual informations.
 * @var LLNode::next
 * Pointer to the next node of the list.
 */
struct LLNode {
	struct ClientInfo client_info;
	struct LLNode *next;
};

/**
 * @struct LinkedList
 *
 * @brief Linked list structure.
 *
 * @var LinkedList::head
 * Pointer to the first node of the list.
 * @var LinkedList::tail
 * Pointer to the last node of the list.
 * @var LinkedList::size
 * Number of nodes in the list.
 */
struct LinkedList {
	struct LLNode *head, *tail;
	int size;
};

/**
 * @brief Compare two \c ClientInfo struct, checking if they share the same
 * connection socket.
 *
 * @param a
 * Pointer to the first ClientInfo struct.
 * @param b
 * Pointer to the second ClientInfo struct.
 *
 * @return \c 0 if they have the same connection socket.
 */
int compare(struct ClientInfo *a, struct ClientInfo *b);

/**
 * @brief Initialize an empty list.
 *
 * @param ll
 * Pointer to the linked list.
 */
void list_init(struct LinkedList *ll);

/**
 * @brief Insert a new element to the list.
 *
 * @param ll
 * Pointer to the linked list.
 * @param cl_info
 * Pointer to the \c ClientInfo structure that will be contained in the new
 * node.
 *
 * @return \c 0 if successful, \c -1 if the list is full or an error occours.
 */
int list_insert(struct LinkedList *ll, struct ClientInfo *cl_info);

/**
 * @brief Delete a node from the list.
 *
 * @param ll
 * Pointer to the linked list.
 * @param cl_info
 * Pointer to the \c ClientInfo structure of the node that has to
 * be removed.
 *
 * @return \c 0 if successful, \c -1 if the list is empty or an error occours.
 */
int list_delete(struct LinkedList *ll, struct ClientInfo *cl_info);

/**
 * @brief Print the list in a readable format.
 *
 * @param ll
 * Pointer to the linked list.
 */
void list_dump(struct LinkedList *ll);

/**
 * @brief Returns the size of the linked list.
 *
 * @param ll
 * Pointer to the linked list.
 *
 * @return An \c int representing the number of nodes in the list.
 */
int list_size(struct LinkedList *ll);

/**
 * @brief Returns an array of strings containing the client's aliases.
 *
 * This method allocate the memory necessary to store the list, so make sure to
 * deallocate using the pointer returned.
 *
 * @param ll
 * Pointer to the linked list.
 *
 * @return An array of strings containing the aliases field of the objects
 * \c ClientInfo in the list.
 */
char **list_clients(struct LinkedList *ll);
