#include "../fs/operations.h"
#include "state.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void *create_inode_FILE(){
    for (int i = 0; i < 100; i++){
        int val = inode_create(T_FILE);
        if (val == -1){
            printf("exceceu max\n");
            printf("%d",i);
            break;
        }
    }
    return 0;
}

void *create_inode_DIR(){
    sleep(1/10);
    for (int i = 0; i < 64; i++){
        inode_delete(i);
    }
    return 0;
}

int main() {

    assert(tfs_init(NULL) != -1);

    
    // Write to file
    pthread_t t1, t2;
    int val1 = pthread_create(&t1, NULL, &create_inode_FILE, NULL);
    assert(val1 == 0);
    int val2 = pthread_create(&t2, NULL, &create_inode_DIR, NULL);
    assert(val2 == 0);

    val1 = pthread_join(t1, NULL);
    assert(val1 == 0);

    val2 = pthread_join(t2,NULL);
    assert(val2 == 0);

    printf("Successful test.\n");
}
