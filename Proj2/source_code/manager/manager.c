#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>  

#define FIFO_SERVER "server.fifo"
#define MAX_SERVER_REGISTER (292)
#define MAX_MESSAGE (1024)
#define MAX_SERVER_MESSAGE (1028)
#define MAX_ANSWER_SERVER (1032)
#define SUBSCRIBER_CODE (1)

#define CREATE_COMMAND "create"
#define REMOVE_COMMAND "remove"
#define LIST_COMMAND "list"
#define MAX_SIZE_COMMAND (8)

#define CREATE_SEND_CODE (3)
#define REMOVE_SEND_CODE (5)
#define LIST_SEND_CODE (7)

//QUESTIONS como dizer ao cliente que a sua ligação foi aceite?  fechar o pipe no open
//QUESTIONS o que acontece quando o maximo de sessões são excedidas?
//QUESTIONS vamos ter max_sessions threads a tratar de clientes?

//QUESTIONS o que acontece qd temos varios processos a escrever no mesmo pipe?
//QUESTIONS como dizer a thread que esta a tratar do cliente que este se vai desligar?

int main(int argc, char **argv) {

    assert(argc == 5 || argc == 4);

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
    
    int code;


    if (!strcmp(argv[3], CREATE_COMMAND)){
        code = CREATE_SEND_CODE;
    }
    else if (!strcmp(argv[3], REMOVE_COMMAND)){
        code = REMOVE_SEND_CODE;
    }
    else {
        code = LIST_SEND_CODE;
    }

    
    //create message to send to server
        // message format: [ code = 3/5 ] | [ client_named_pipe_path (char[256])] | [ box_name (32)]
        // or [code = 7 ] | [ client_named_pipe_path (char[256])]

    char *register_message = malloc(MAX_SERVER_REGISTER);
    memset(register_message, 0, MAX_SERVER_REGISTER);

    sprintf(register_message, "%04d", code);

    memcpy(register_message + 4, argv[2], strlen(argv[2]));
    
    if (code != LIST_SEND_CODE){
        memcpy(register_message + 260, argv[4], strlen(argv[4]));
    }
    
    assert(write(register_FIFO, register_message, MAX_SERVER_REGISTER) == MAX_SERVER_REGISTER);

    //opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    //QUESTIONS meto o fprintf?
    if (client_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(MAX_ANSWER_SERVER);
    memset(buffer, 0, MAX_ANSWER_SERVER);

    
    //reads message
    //  message format: [ code = 4/6 ] | [ return_code (int32_t)] | [ message (char[1024]) ]
    //  or [ code = 8 ] | [ last (uint8_t)] | [ box_name char(32) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
    while(read(client_FIFO, buffer, MAX_ANSWER_SERVER) != 0){

        fprintf(stdout, "%s\n", buffer);
        memset(buffer, 0, MAX_ANSWER_SERVER);
    }

    close(client_FIFO);
    close(register_FIFO);

    //remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
