#ifndef __MBROKER_H__
#define __MBROKER_H__
#include "producer-consumer.h"

typedef struct{
    char *name;
    int number_subscribers;
    int number_publishers;
    pthread_mutex_t box_lock;
    pthread_cond_t box_cond;
} box;

typedef struct {
    pc_queue_t *queue;
    box *boxes;
    pthread_rwlock_t boxes_lock;
    int box_size;
} m_broker_values;

#endif // __MBROKER_H__