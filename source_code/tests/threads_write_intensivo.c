#include "../fs/operations.h"
#include "state.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMBER_THREADS (40)

size_t write_len = 2;
char write_contets[] = "AA";
char path_name[] = "/f1";
char full_contets[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";



void *threads_write(void *transport){
    int *fd = (int*) transport;
    assert(tfs_write(*fd, write_contets, write_len) == write_len);
    return 0;
}

int main(){
    assert(tfs_init(NULL) != -1);       //inicializa o tfs

    pthread_t *threads = malloc(sizeof(pthread_t) * NUMBER_THREADS * 2);        //inicializa uma tabela de threads

    int fd = tfs_open(path_name, TFS_O_CREAT);      //cria um ficheiro path_name

    int *transport = malloc(sizeof(int));       //transporta o file_handle do tfs_open
    *transport = fd;

    for (int i = 0; i < NUMBER_THREADS; i++){
        assert(pthread_create(&(threads[i]), NULL, &threads_write, (void*)transport) == 0);     //cria 40 threads que escrevem ao mesmo tempo
    }

    for (int i = 0; i < NUMBER_THREADS; i++){
            assert(pthread_join(threads[i], NULL) == 0);        //espera que as 40 threads dêm return
    }

    assert(tfs_close(fd) != -1);        //fecha o ficheiro


    char *answer = malloc(sizeof(char) * NUMBER_THREADS * 2);       //criar o resultado esperado

    fd = tfs_open(path_name, TFS_O_CREAT);      //abre o ficheiro para ler

    assert(tfs_read(fd, answer, NUMBER_THREADS * 2) == NUMBER_THREADS * 2);     //le o ficheiro

    assert(tfs_close(fd) != -1);        //fecha o ficheiro

    assert(memcmp(answer,full_contets,NUMBER_THREADS * 2) == 0);        //verifica o resultado obtido
    assert(tfs_destroy() != -1);        //fecha o tfs

    printf("Successful test.\n");
    
    return 0; 
}

