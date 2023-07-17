#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

typedef struct list_node {
    struct list_node* next; // next node in list
    struct list_node* prev; // prev node in list
    void* data; // generic user-specified data pointer
} list_node_t;

typedef struct {
    list_node_t* head; // head of the list
    list_node_t* tail; // tail of the list
    size_t count; // count of nodes in the list
} list_t;

// Creates and returns a new list
list_t* list_create();

// Destroys a list
void list_destroy(list_t* list);

// Returns head of the list
list_node_t* list_head(list_t* list);

// Returns tail of the list
list_node_t* list_tail(list_t* list);

// Returns next element in the list
list_node_t* list_next(list_node_t* node);

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node);

// Returns end of the list marker
list_node_t* list_end(list_t* list);

// Returns data in the given list node
void* list_data(list_node_t* node);

// Returns the number of elements in the list
size_t list_count(list_t* list);

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data);

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data);

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node);

#endif // LINKED_LIST_H
