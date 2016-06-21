/* Necessary for the definition of the struct ClientInfo */
#include "networkdef.h"

/* Node of the list */
struct LLNode {
	struct ClientInfo client_info;
	struct LLNode *next;
};

/* List */
struct LinkedList {
	struct LLNode *head, *tail;
	int size;
};

/* Compare two ClientInfo struct, return 0 if they have the same connection socket */
int compare(struct ClientInfo *a, struct ClientInfo *b);

/* Initialize an empty list */
void list_init(struct LinkedList *ll);

/* Insert an element to the list, return -1 if the list is full */
int list_insert(struct LinkedList *ll, struct ClientInfo *cl_info);

/* Delete a client node from the list, return -1 if the element isn't found */
int list_delete(struct LinkedList *ll, struct ClientInfo *cl_info);

/* Print the clients in a readable format */
void list_dump(struct LinkedList *ll);

/* Returns the size of the linked list */
int list_size(struct LinkedList *ll);

/* Returns an array of strings containing the client's aliases.
This method allocate the memory necessary to store the list, so make sure to deallocate
using the pointer returned*/
char **list_clients(struct LinkedList *ll);
