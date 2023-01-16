#include "producer-consumer.h"
#include <stdlib.h>
// This function creates a producer-consumer queue with a specified capacity.
int pcq_create(pc_queue_t *queue, size_t capacity) {
    // Allocate memory for the buffer
    queue->pcq_buffer = malloc(capacity * sizeof(void *));
    // If malloc fails, return -1
    if (queue->pcq_buffer == NULL) {
        return -1;
    }
    // Set capacity, current size, head and tail pointers
    queue->pcq_capacity = capacity;

    queue->pcq_current_size = 0;

    queue->pcq_head = 0;

    queue->pcq_tail = 0;
    // Initialize mutexes and condition variables
    pthread_mutex_init(&(queue->pcq_current_size_lock), NULL);

    pthread_mutex_init(&(queue->pcq_head_lock), NULL);

    pthread_mutex_init(&(queue->pcq_tail_lock), NULL);

    pthread_mutex_init(&(queue->pcq_pusher_condvar_lock), NULL);

    pthread_mutex_init(&(queue->pcq_popper_condvar_lock), NULL);

    pthread_cond_init(&(queue->pcq_pusher_condvar), NULL);

    pthread_cond_init(&(queue->pcq_popper_condvar), NULL);

    return 0;
}
// This function frees the resources used by the queue.
int pcq_destroy(pc_queue_t *queue) {

    free(queue->pcq_buffer);

    queue->pcq_capacity = 0;
    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&(queue->pcq_current_size_lock));

    pthread_mutex_destroy(&(queue->pcq_head_lock));

    pthread_mutex_destroy(&(queue->pcq_tail_lock));

    pthread_mutex_destroy(&(queue->pcq_pusher_condvar_lock));

    pthread_mutex_destroy(&(queue->pcq_popper_condvar_lock));

    pthread_cond_destroy(&(queue->pcq_pusher_condvar));

    pthread_cond_destroy(&(queue->pcq_popper_condvar));

    return 0;
}
// This function adds an element to the queue.
int pcq_enqueue(pc_queue_t *queue, void *elem) {

    pthread_mutex_lock(&(queue->pcq_tail_lock));

    pthread_mutex_lock(&(queue->pcq_popper_condvar_lock));
    while (1) {
        pthread_mutex_lock(&(queue->pcq_current_size_lock));
        // If the queue is not full, break out of the while loop
        if (queue->pcq_current_size < queue->pcq_capacity) {
            break;
        }
        pthread_mutex_unlock(&(queue->pcq_current_size_lock));
        // If the queue is full, wait for a signal on the popper condition
        // variable
        pthread_cond_wait(&(queue->pcq_popper_condvar),
                          &(queue->pcq_popper_condvar_lock));
    }
    pthread_mutex_unlock(&(queue->pcq_popper_condvar_lock));
    // Increase the current size of the queue
    queue->pcq_current_size += 1;
    pthread_mutex_unlock(&(queue->pcq_current_size_lock));
    // Add the element to the buffer
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_tail += 1;

    if (queue->pcq_tail > queue->pcq_capacity) {
        queue->pcq_tail = 0;
    }

    pthread_cond_signal(&(queue->pcq_pusher_condvar));

    pthread_mutex_unlock(&(queue->pcq_tail_lock));

    return 0;
}
// This function removes an element from the queue.
void *pcq_dequeue(pc_queue_t *queue) {

    pthread_mutex_lock(&(queue->pcq_head_lock));

    pthread_mutex_lock(&(queue->pcq_pusher_condvar_lock));
    while (1) {
        pthread_mutex_lock(&(queue->pcq_current_size_lock));
        // If the queue is not empty, break out of the while loop
        if (queue->pcq_current_size > 0) {
            break;
        }
        pthread_mutex_unlock(&(queue->pcq_current_size_lock));
        // If the queue is empty, wait for a signal on the pusher condition
        // variable
        pthread_cond_wait(&(queue->pcq_pusher_condvar),
                          &(queue->pcq_pusher_condvar_lock));
    }
    pthread_mutex_unlock(&(queue->pcq_pusher_condvar_lock));

    queue->pcq_current_size -= 1;
    pthread_mutex_unlock(&(queue->pcq_current_size_lock));
    // Get the element from the buffer
    void *elem = queue->pcq_buffer[queue->pcq_head];

    queue->pcq_head += 1;

    if (queue->pcq_head > queue->pcq_capacity) {
        queue->pcq_head = 0;
    }

    pthread_cond_signal(&(queue->pcq_popper_condvar));

    pthread_mutex_unlock(&(queue->pcq_head_lock));

    return elem;
}