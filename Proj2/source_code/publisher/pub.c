#include "logging.h"
#include "betterassert.h"
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

#define MAX_SERVER_REGISTER (292)
#define MAX_MESSAGE (1024)
#define MAX_SERVER_MESSAGE (1028)
#define PUBLISHER_REGISTER_CODE (2)
#define PUBLISHER_MESSAGE_CODE (10)
    //FIXME verificar tamanho dos args

int main(int argc, char **argv) {

    assert(argc == 4);

    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");

    //create FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
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
    // message format: [ code = 2 ] | [ client_named_pipe_path (char[256])] | [ box_name (32)]
    void *register_message = malloc(MAX_SERVER_REGISTER);
    memset(register_message, 0, MAX_SERVER_REGISTER);
    uint8_t register_code = PUBLISHER_REGISTER_CODE;
    memcpy(register_message, &register_code,sizeof(uint8_t));
    memcpy(register_message + sizeof(uint8_t), argv[2], strlen(argv[2]));
    memcpy(register_message + sizeof(uint8_t) +265, argv[3], strlen(argv[3]));
    
    assert(write(register_FIFO, register_message, MAX_SERVER_REGISTER) == MAX_SERVER_REGISTER);

    //opens clients FIFO
    int client_FIFO = open(argv[2], O_WRONLY);

    //QUESTIONS meto o fprintf?
    if (client_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(MAX_MESSAGE);
    memset(buffer, 0, MAX_MESSAGE);

    char *server_command = malloc(MAX_SERVER_MESSAGE);
    
    //QUESTIONS e so \n fazemos oq?
    //QUESTIONS se acabar em \0 termina?
    uint8_t publisher_message_code = PUBLISHER_MESSAGE_CODE;
    while(fgets(buffer, MAX_MESSAGE, stdin) != NULL){
        buffer = strtok(buffer, "\n");
        size_t length = strlen(buffer);
        if (!length){
            break;
        }

        printf("%s\n", buffer);

        //setting up command to send to server
        //  command format: [ code = 10 ] | [ message (char[1024]) ]
        
        memset(server_command, 0, MAX_SERVER_MESSAGE);
        memset(server_command, &publisher_message_code, sizeof(uint8_t));
        memcpy(server_command+ sizeof(uint8_t), buffer, MAX_MESSAGE);

        ALWAYS_ASSERT(write(client_FIFO, server_command, MAX_SERVER_MESSAGE) != -1, "error writing message");
        //FIXME just for testing
    }

    close(client_FIFO);
    close(register_FIFO);

    //remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
