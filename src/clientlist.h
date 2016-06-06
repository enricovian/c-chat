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
int compare(struct ClientInfo *a, struct ClientInfo *b) {
	return a->sockfd - b->sockfd;
}

/* Initialize an empty list */
void list_init(struct LinkedList *ll) {
	ll->head = ll->tail = NULL;
	ll->size = 0;
}

/* Insert an element to the list, return -1 if the list is full */
int list_insert(struct LinkedList *ll, struct ClientInfo *cl_info) {
	if(ll->size == MAXCLIENTS) return -1; // check if the list is full
	/* If the list is empty, create the node and make head and tail point to it */
	if(ll->head == NULL) {
		ll->head = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->head->client_info = *cl_info;
		ll->head->next = NULL;
		ll->tail = ll->head;
	}
	/* If the list isn't empty, create a new node and make the tail point to it */
	else {
		ll->tail->next = (struct LLNode *)malloc(sizeof(struct LLNode));
		ll->tail->next->client_info = *cl_info;
		ll->tail->next->next = NULL;
		ll->tail = ll->tail->next;
	}
	ll->size++;
	return 0;
}

/* Delete a client node from the list, return -1 if the element isn't found */
int list_delete(struct LinkedList *ll, struct ClientInfo *cl_info) {
	struct LLNode *curr, *tmp;
	if(ll->head == NULL) return -1; // check if the structure is empty
	/* Handle the case where the element to remove is the first */
	if(!compare(cl_info, &ll->head->client_info)) {
		tmp = ll->head; // store the element in a temporary variable to free the memory
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
			tmp = curr->next; // store the element in a temporary variable to free the memory
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

/* Print the clients in a readable format */
void list_dump(struct LinkedList *ll) {
	struct LLNode *curr;
	struct ClientInfo *cl_info;
	printf("Connection count: %d\n", ll->size);
	for(curr = ll->head; curr != NULL; curr = curr->next) {
		cl_info = &curr->client_info;
		printf("[%d] %s\n", cl_info->sockfd, cl_info->alias);
	}
}
