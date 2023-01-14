#ifndef __MBROKER_H__
#define __MBROKER_H__
#include "producer-consumer.h"
#include <stdint.h>

struct box{
    char *name;
    uint64_t number_subscribers;
    uint64_t number_publishers;
    pthread_mutex_t box_lock;
    pthread_cond_t box_cond;
    uint64_t box_size;
    struct box *next;
};

typedef struct {
    pc_queue_t *queue;
    struct box *boxes_head;
    pthread_rwlock_t boxes_lock;
    int num_box;
} m_broker_values;

struct box *newBox(char *name);

void insertBox(struct box **head, char *name);

struct box*getBox(struct box **head, char *name);




#endif // __MBROKER_H__