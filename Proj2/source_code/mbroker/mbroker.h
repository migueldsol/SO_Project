#ifndef __MBROKER_H__
#define __MBROKER_H__
#include "producer-consumer.h"
#include <stdint.h>

typedef struct {
    char *name;
    uint64_t number_subscribers;
    uint64_t number_publishers;
    pthread_mutex_t box_lock;
    pthread_cond_t box_cond;
    uint64_t box_size;
} box;

typedef struct {
    pc_queue_t *queue;
    box *boxes;
    pthread_rwlock_t boxes_lock;
    int num_box;
} m_broker_values;

#endif // __MBROKER_H__