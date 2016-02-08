/*
 * Generic queue manipulation functions
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "host_controller.h"
/*
 * queue_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how queues are
 * represented.  They see and manipulate only queue_t's.
 */

// Represents each node in the linked queue structure.
typedef struct entity {
    // The command
	P_HOST_CMD cmd;
    // The next entity in the queue (from front to back).
    struct entity *next;
} entity_t;

// Implementation for a FIFO queue with
// O(1) prepend, append, and dequeue.
typedef struct queue {
    entity_t *head;
    entity_t *tail;
    int length;
} queue_t;

/*
 * Return an empty queue.  Returns NULL on error.
 */
queue_t* queue_new();

/*
 * Prepend a void* to a queue (both specifed as parameters).
 * Returns 0 (success) or -1 (failure).
 */
int queue_prepend(queue_t*, P_HOST_CMD);

/*
 * Appends a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int queue_append(queue_t*, P_HOST_CMD);

/*
 * Dequeue and return the first void* from the queue.
 * Return 0 (success) and first item if queue is nonempty, or -1 (failure) and
 * NULL if queue is empty.
 */
int queue_dequeue(queue_t*, P_HOST_CMD*);

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int queue_free (queue_t*);

/*
 * Return the number of items in the queue, or -1 if an error occured
 */
int queue_length(const queue_t* queue);

#endif /*__QUEUE_H__*/
