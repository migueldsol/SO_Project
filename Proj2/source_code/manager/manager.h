#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>
struct Node{
    char *box_name;
    uint8_t last;
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers; 
    struct Node *next;
};

struct Node *newNode(uint8_t last, char *box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subcribers);

void insertAlreadySorted(struct Node **head, uint8_t last, char *box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subcribers);

#endif // __MANAGER_H__