#include "../fs/operations.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *file_names[41];

void *create_files(){
    for(int i = 0; i<20;i++){
        file_names[i] = malloc(sizeof(char) * 5);
        sprintf(file_names[i],"/f%d",i);
    }
    return 0;
}

void *add_file(void *path_name){
    int fd = tfs_open((char*) path_name, TFS_O_CREAT);
    assert(tfs_close(fd) != -1);
    return 0;
}

void *unlink_file(void *path){
    assert(tfs_unlink((char*)path) != -1);
    return 0;
}

int main(){
    create_files();
    assert(tfs_init(NULL) != -1);
    pthread_t *threads = malloc(sizeof(pthread_t) * 41);
    for(int i = 0; i < 20; i++){
        assert(pthread_create(&(threads[i]), NULL, &add_file, (void*)file_names[i]) == 0);
    }
    for(int i = 0; i < 20; i++){
        assert(pthread_join(threads[i], NULL) == 0);
    }
    for(int i = 0; i < 20; i++){
        assert(pthread_create(&(threads[i]), NULL, &unlink_file, (void*)file_names[i]) == 0);
    }
    for(int i = 0; i < 20; i++){
        assert(pthread_join(threads[i], NULL) == 0);
    }
    
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
    
}