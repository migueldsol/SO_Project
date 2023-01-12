#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>  

#define FIFO_SERVER "server.fifo"
#define MAX_BOX_NAME (32)
#define MAX_PIPE_NAME (256)
#define MAX_MESSAGE (1024)
#define MAX_PUB_SUB_MESSAGE (1028)
#define SUBSCRIBER_CODE (2)
#define UINT8_T_SIZE (12)

//QUESTIONS uint_8

int main(int argc, char **argv) {
    //FIXME verificar tamanho dos args
    assert(argc == 4);

    //create FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(mkfifo(argv[2], 0640) != 0){
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //open server FIFO
    int register_FIFO = open(argv[1], O_WRONLY);

    //QUESTIONS meto o fprintf?
    if (register_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //create message to send to server
    // 0001ola.fifo0\0\0\0\0....\0
    // message format: [ code = 1 (uint8_t)] | [ client_named_pipe_path (char[256])] | [ box_name (32)]

    //TODO macros
    void *register_message = malloc(MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);
    uint8_t code = SUBSCRIBER_CODE;
    memset(register_message, 0, MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);
    memcpy(register_message, &code, UINT8_T_SIZE);
    memcpy(register_message + UINT8_T_SIZE, argv[2], strlen(argv[2]));
    memcpy(register_message + UINT8_T_SIZE + MAX_PIPE_NAME, argv[3], strlen(argv[3]));
    
    assert(write(register_FIFO, register_message, MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE) == MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);

    //opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    //QUESTIONS meto o fprintf?
    if (client_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(MAX_PUB_SUB_MESSAGE);
    memset(buffer, 0, MAX_PUB_SUB_MESSAGE);

    

    //reads message
    //  message format: [ code = 10 ] | [ message (char[1024]) ]
    while(read(client_FIFO, buffer, MAX_PUB_SUB_MESSAGE) != 0){
        if (strlen(buffer + UINT8_T_SIZE) == 0){
            break;
        }
        fprintf(stdout, "%s\n", buffer + UINT8_T_SIZE);
    }

    close(client_FIFO);
    close(register_FIFO);

    //remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    free(register_message);
    free(buffer);

    return 0;
}
