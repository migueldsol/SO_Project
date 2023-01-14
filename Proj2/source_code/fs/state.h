#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define FREE_BLOCK_MUTEX_ENTRIE (0)
#define OPEN_FILE_MUTEX_ENTRIE (1)
#define INODE_MUTEX_ENTRIE (2)
#define FREEINODE_MUTEX_ENTRIE (3)

/**
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY, T_SYMLINK } inode_type;

/**
 * Inode
 */
typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block;
    int hard_link;
    int open_inode;
    pthread_rwlock_t rw_lock;
    pthread_mutex_t cond_lock;
    pthread_cond_t cond;

    // in a more complete FS, more fields could exist here
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

/**
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
} open_file_entry_t;

int state_init(tfs_params);
int state_destroy(void);

size_t state_block_size(void);

int inode_create(inode_type n_type);
void inode_delete(int inumber);
inode_t *inode_get(int inumber);

int clear_dir_entry(inode_t *inode, char const *sub_name);
int add_dir_entry(inode_t *inode, char const *sub_name, int sub_inumber);
int find_in_dir(inode_t const *inode, char const *sub_name);

int data_block_alloc(void);
void data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset);
void remove_from_open_file_table(int fhandle);
open_file_entry_t *get_open_file_entry(int fhandle);

pthread_mutex_t *get_mutex_table();
void pthread_m_lock(int type);
void pthread_m_unlock(int type);
void pthread_write_lock(inode_t *inode);
void pthread_wr_unlock(inode_t *inode);
void pthread_read_lock(inode_t *inode);

void pthread_mutex_inode_lock(inode_t *inode);
void pthread_mutex_inode_unlock(inode_t *inode);
void pthread_inode_cond_wait(inode_t *inode);
void pthread_inode_cond_signal(inode_t *inode);
#endif // STATE_H
