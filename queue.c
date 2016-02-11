/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

// Macros for determining which operation 
// is occurring in the call to add_entity().
#define APPEND 1
#define PREPEND 0

/*
 * Return an empty queue.
 */
queue_t*
queue_new() {
    queue_t *queue = (queue_t*) malloc(sizeof(queue_t));
    if (queue == NULL) {
        return NULL;
    }
    // Initialize the values in the queue.
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    return queue;
}

/*
 * Adds an entity with value "item" to the queue 
 * depending on the value of "append".
 */
int
add_entity(queue_t *queue, P_HOST_CMD c, const unsigned int append) {
    // Cannot add an entity to a NULL queue nor from a NULL item.
    if (queue == NULL || c == NULL) {
        return -1;
    }
    // entity_t *entity = (entity_t*) malloc(sizeof(entity_t));
    //if (entity == NULL) {
    //    return -1;
    //}

    entity_t entity;
    entity.cmd = c;
    if (queue->length == 0) {
        // Set the head and tail pointers in the queue
        // if this is the first item in the queue.
        entity.next = NULL;
        queue->head = queue->tail = &entity;
    } else if (append) {
        // Append the entity to the queue.
        entity.next = NULL;
        queue->tail->next = &entity;
        queue->tail = &entity;
    } else {
        // Prepend the entity to the queue.
        entity.next = queue->head;
        queue->head = &entity;
    }
    queue->length++;
    return 0;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t *queue, P_HOST_CMD c) {
    return add_entity(queue, c, PREPEND);
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
queue_append(queue_t *queue, P_HOST_CMD c) {
    return add_entity(queue, c, APPEND);
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
queue_dequeue(queue_t *queue, P_HOST_CMD* c) {
    // Cannot dequeue from a NULL queue nor place the value in a NULL item.
    if (queue == NULL || c == NULL) {
        return -1;
    }
    if (queue->length == 0) { // The queue is empty.
        *c = NULL;
        return -1;
    }
    entity_t *entity = queue->head;
    // Re-set the head pointer.
    if (queue->length == 1) {
        // The only entity in the queue is removed,
        // so head and tail point to NULL.
        queue->head = queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }
    queue->length--;
    *c = entity->cmd;
    //free(entity);
    return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int
queue_free (queue_t *queue) {
    // Cannot free a NULL queue.
    if (queue == NULL) {
        return -1;
    }
    entity_t *curr = queue->head;
    entity_t *tmp;
    while (curr != NULL) {
        tmp = curr->next;
        free(curr);
        curr = tmp;
    }
    free(queue);
    return 0;
}

/*
 * Return the number of items in the queue.
 */
int
queue_length(const queue_t *queue) {
    // A NULL queue doesn't not have a length.
    if (queue == NULL) {
        return -1;
    }
    return queue->length;
}

