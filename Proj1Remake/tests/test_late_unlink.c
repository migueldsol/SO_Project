#include "../fs/operations.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

size_t write_len = 13;
char write_contets[] = "Hello world!";


void *thread_long_open(void *path_name){
    int fd = tfs_open((char*)path_name, 0);     //abre o ficheiro
    assert(fd != -1);
    sleep(10);                                  //espera por 10 segundos
    assert(tfs_close(fd) != -1);                //fecha o ficheiro
    return 0;
}

void *thread_long_unlink(void *path_name){
    assert(tfs_unlink((char*)path_name) != -1);
    return 0;
}
int main(){
    assert(tfs_init(NULL) != -1);       //inicializa o tfs
    pthread_t open, unlink;      //cria 2 threads
    char path_name[] = "/f1";
    int fd = tfs_open(path_name, TFS_O_CREAT);             //cria um ficheiro
    assert(fd != -1);                                                 //verifica se abriu o ficheiro
    assert(tfs_write(fd, write_contets,write_len) == write_len);        //escreve no ficheiro
    assert(tfs_close(fd) != -1);        //fecha o ficheiro

    assert(pthread_create(&(open), NULL, &thread_long_open, (void*)path_name) == 0);  
    assert(pthread_create(&(unlink), NULL, &thread_long_unlink, (void*)path_name) == 0);

    assert(pthread_join(open, NULL) == 0);        //espera que estas retornem
    assert(pthread_join(unlink, NULL) == 0);        //espera que estas retornem

        
    assert(tfs_destroy() != -1);        //fecha o tfs

    printf("Successful test.\n");
    
    return 0; 
}