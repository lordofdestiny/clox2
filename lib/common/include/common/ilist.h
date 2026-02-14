#ifndef __CLOX_ILIST_H__
#define __CLOX_ILIST_H__

#include <stdlib.h>

typedef struct ilist_node_t ilist_node_t; 

#define ILIST_HEAD_INIT(head) (ilist_node_t){&(head), &(head)}

#define container_of(ptr, type, member) \
    (type*)((char*)(ptr) - offsetof(type, member))
    

struct ilist_node_t {
    ilist_node_t* next;
    ilist_node_t* prev;
};

typedef struct  {
    ilist_node_t head;
} ilist_t;

static inline void ilist_init(ilist_t * list) {
    list->head = ILIST_HEAD_INIT(list->head);
}

static inline void ilist_add_between(
	ilist_node_t *prev,
	ilist_node_t *next,
    ilist_node_t *new
) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void ilist_add_front(ilist_t* list, ilist_node_t* node) {
    ilist_add_between( &list->head, list->head.next, node);
}

static inline void ilist_add_back(ilist_t* list, ilist_node_t* node) {
    ilist_add_between(list->head.prev, &list->head , node);
}

static inline void ilist_remove_between(ilist_node_t * prev, ilist_node_t * next) {
	next->prev = prev;
	prev->next = next;
}

static inline void ilist_remove(ilist_node_t* node) {
    ilist_remove_between(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
}

static inline void list_replace(ilist_node_t *node, ilist_node_t * with) {
    with->next = node->next;
	with->next->prev = with;
	with->prev = node->prev;
	with->prev->next = with;
}

#define ILIST_FOR_EACH(head, node) \
    for (node = (head).next; node != &(head); node = node->next)

#endif // __CLOX_ILIST_H__
