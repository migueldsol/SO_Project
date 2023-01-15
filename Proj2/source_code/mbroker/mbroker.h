#ifndef __MBROKER_H__
#define __MBROKER_H__
#include "producer-consumer.h"
#include <stdint.h>

struct box{
    char *name;
    pthread_t *publisher_thread;
    uint64_t number_subscribers;
    uint64_t number_publishers;
    pthread_mutex_t box_lock;
    pthread_cond_t box_cond;
    uint64_t box_size;
    struct box *next;
};

typedef struct {
    pc_queue_t *queue;
    struct box **boxes_head;
    pthread_rwlock_t boxes_lock;
    int num_box;
} m_broker_values;

struct box *newBox(char *name);

void insertBox(m_broker_values *mbroker, char *name);

struct box*getBox(m_broker_values *mbroker, char *name);


void pthread_write_lock_broker(m_broker_values *broker);
void pthread_wr_unlock_broker(m_broker_values *broker);
void pthread_read_lock_broker(m_broker_values *broker);

void pthread_box_broadcast(struct box *some_box);
void pthread_box_wait(struct box* some_box);
void pthread_box_lock(struct box *some_box);
void pthread_box_unlock(struct box *some_box);

#endif // __MBROKER_H__