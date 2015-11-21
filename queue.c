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
add_entity(queue_t *queue, void *item, const unsigned int append) {
    // Cannot add an entity to a NULL queue nor from a NULL item.
    if (queue == NULL || item == NULL) {
        return -1;
    }
    entity_t *entity = (entity_t*) malloc(sizeof(entity_t));
    if (entity == NULL) {
        return -1;
    }
    entity->value = item;
    if (queue->length == 0) {
        // Set the head and tail pointers in the queue
        // if this is the first item in the queue.
        entity->next = NULL;
        queue->head = queue->tail = entity;
    } else if (append) {
        // Append the entity to the queue.
        entity->next = NULL;
        queue->tail->next = entity;
        queue->tail = entity;
    } else {
        // Prepend the entity to the queue.
        entity->next = queue->head;
        queue->head = entity;
    }
    queue->length++;
    return 0;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t *queue, void *item) {
    return add_entity(queue, item, PREPEND);
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
queue_append(queue_t *queue, void *item) {
    return add_entity(queue, item, APPEND);
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
queue_dequeue(queue_t *queue, void **item) {
    // Cannot dequeue from a NULL queue nor place the value in a NULL item.
    if (queue == NULL || item == NULL) {
        return -1;
    }
    if (queue->length == 0) { // The queue is empty.
        *item = NULL;
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
    *item = entity->value;
    free(entity);
    return 0;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its second
 * argument and the queue element is the first.  Return 0 (success)
 * or -1 (failure).
 */
int
queue_iterate(queue_t *queue, func_t f, void* item) {
    // Cannot iterate through a NULL queue nor call a NULL function.
    if (queue == NULL || f == NULL) {
        return -1;
    }
    entity_t *curr = queue->head;
    while (curr != NULL) {
        f(curr->value, item);
        curr = curr->next;
    }
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

/*
 * Delete the specified item from the given queue.
 * Return -1 on error.
 */
int
queue_delete(queue_t *queue, void* item) {
    // Cannot delete an item from a NULL or empty queue, nor delete a NULL item.
    if (queue == NULL || item == NULL || queue->length == 0) {
        return -1;
    }
    if (queue->head->value == item) {
        entity_t *entity = queue->head;
        // Re-set the head pointer if it is deleted.
        queue->head = queue->head->next;
        if (queue->head == NULL) {
            // Just deleted the only item in the queue.
            queue->tail = NULL;
        }
        free(entity);
        entity = NULL;
        queue->length--;
        return 0;
    }
    entity_t *prev = queue->head;
    entity_t *curr = queue->head->next;
    while (curr != NULL) {
        if (curr->value == item) {
            prev->next = curr->next;
            if (curr == queue->tail) {
                // Re-set the tail pointer if it is deleted.
                queue->tail = prev;
            }
            free(curr);
            curr = NULL;
            queue->length--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}
