/**
 * @file clientlist.c
 * @brief Linked list implementation where every node represent a connection
 * with a client.
 *
 * @author Enrico Vianello (<enrico.vianello.1@studenti.unipd.it>)
 * @version 1.0
 * @since 1.0
 *
 * @copyright Copyright (c) 2016-2017, Enrico Vianello
 */

#include "clientlist.h"

/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int compare(struct ClientInfo *a, struct ClientInfo *b) {
	return a->sockfd - b->sockfd;
}

/**
 * @brief Initialize an empty list.
 *
 * @param ll
 * Pointer to the linked list.
 */
void list_init(struct LinkedList *ll) {
	ll->head = ll->tail = NULL;
	ll->size = 0;
}

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
int list_insert(struct LinkedList *ll, struct ClientInfo *cl_info) {
	if(ll->size == MAXCLIENTS) return -1; // check if the list is full
	/* If the list is empty, create the node and make head and tail
	point to it */
	if(ll->head == NULL) {
		ll->head = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->head->client_info = *cl_info;
		ll->head->next = NULL;
		ll->tail = ll->head;
	}
	/* If the list isn't empty, create a new node and make the tail
	point to it */
	else {
		ll->tail->next = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->tail->next->client_info = *cl_info;
		ll->tail->next->next = NULL;
		ll->tail = ll->tail->next;
	}
	ll->size++;
	return 0;
}

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
int list_delete(struct LinkedList *ll, struct ClientInfo *cl_info) {
	struct LLNode *curr, *tmp;
	if(ll->head == NULL) return -1; // check if the structure is empty
	/* Handle the case where the element to remove is the first */
	if(!compare(cl_info, &ll->head->client_info)) {
		/* store the element in a temporary variable to free the memory */
		tmp = ll->head;
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
			/* store the element in a temporary variable to free the memory */
			tmp = curr->next;
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

/**
 * @brief Print the list in a readable format.
 *
 * @param ll
 * Pointer to the linked list.
 */
void list_dump(struct LinkedList *ll) {
	struct LLNode *curr;
	struct ClientInfo *cl_info;
	printf("Connection count: %d\n", ll->size);
	for(curr = ll->head; curr != NULL; curr = curr->next) {
		cl_info = &curr->client_info;
		printf("[%d] %s\n", cl_info->sockfd, cl_info->alias);
	}
}

/**
 * @brief Returns the size of the linked list.
 *
 * @param ll
 * Pointer to the linked list.
 *
 * @return An \c int representing the number of nodes in the list.
 */
int list_size(struct LinkedList *ll) {
	return ll->size;
}

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
char **list_clients(struct LinkedList *ll) {
	char ** list_str = malloc(ll->size * sizeof(char*));
	struct LLNode *curr;
	struct ClientInfo *cl_info;
	int i = 0;
	for(curr = ll->head; curr != NULL; curr = curr->next) {
		list_str[i] = malloc(ALIASLEN * sizeof(char));
		strcpy(list_str[i++], curr->client_info.alias);
	}
	return list_str;
}
