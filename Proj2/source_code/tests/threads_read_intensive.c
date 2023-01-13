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

void *threads_read(void *path_name) {
    int fd = tfs_open((char *)path_name, 0);
    char *read_contets = malloc(write_len);
    assert(tfs_read(fd, read_contets, write_len) == write_len); // le o ficheiro
    assert(memcmp(write_contets, read_contets, write_len) ==
           0); // verifica se a leitura esta correta
    assert(tfs_close(fd) != -1);
    return 0;
}

int main() {
    assert(tfs_init(NULL) != -1); // inicializa o tfs
    pthread_t *threads =
        malloc(sizeof(pthread_t) * 5000); // cria uma tabela de 5000 threads
    char path_name[] = "/f1";
    int fd = tfs_open(path_name, TFS_O_CREAT); // cria um ficheiro
    assert(fd != -1);                          // verifica se abriu o ficheiro
    assert(tfs_write(fd, write_contets, write_len) ==
           write_len);           // escreve no ficheiro
    assert(tfs_close(fd) != -1); // fecha o ficheiro

    for (int i = 0; i < 5000; i++) {
        assert(pthread_create(&(threads[i]), NULL, &threads_read,
                              (void *)path_name) ==
               0); // cria 5000 threads para escrever no ficheiro
    }

    for (int i = 0; i < 5000; i++) {
        assert(pthread_join(threads[i], NULL) == 0); // espera que estas
                                                     // retornem
    }

    assert(tfs_destroy() != -1); // fecha o tfs

    printf("Successful test.\n");

    return 0;
}