#include "../fs/operations.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t write_len = 13;
char write_contets[] = "Hello world!";

void *threads_read(void *path_name){
    int fd = tfs_open((char*) path_name, 0);
    char *read_contets = malloc(write_len);
    assert(tfs_read(fd, read_contets, write_len) == write_len);
    assert(memcmp(write_contets, read_contets, write_len) == 0);
    assert(tfs_close(fd) != -1);
    return 0;
}

int main(){
    assert(tfs_init(NULL) != -1);
    pthread_t *threads = malloc(sizeof(pthread_t) * 5000);
    char path_name[] = "/f1";
    int fd = tfs_open(path_name, TFS_O_CREAT);
    assert(tfs_write(fd, write_contets,write_len) == write_len);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    for (int i = 0; i < 5000; i++){
        assert(pthread_create(&(threads[i]), NULL, &threads_read, (void*)path_name) == 0);
    }
    
    for (int i = 0; i < 5000; i++){
            assert(pthread_join(threads[i], NULL) == 0);
    }
        
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    
    return 0; 
}