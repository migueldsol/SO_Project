#include "logging.h"
#include "betterassert.h"
#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>  

#define FIFO_SERVER "server.fifo"
#define MAX_SERVER_REGISTER (256+32+1)
#define MAX_PIPE_NAME (256)
#define MAX_BOX_NAME (32)
#define MAX_MESSAGE (1024)
#define MAX_SERVER_MESSAGE (1028)
#define MAX_ANSWER_SERVER (1032)
#define SUBSCRIBER_CODE (1)

#define CREATE_COMMAND "create"
#define REMOVE_COMMAND "remove"
#define LIST_COMMAND "list"
#define MAX_SIZE_COMMAND (8)

#define CREATE_SEND_CODE (3)
#define CREATE_RECEIVE_CODE (4)
#define REMOVE_SEND_CODE (5)
#define REMOVE_RECEIVE_CODE (6)
#define LIST_SEND_CODE (7)
#define LIST_RECEIVE_CODE (8)


struct Node *newNode(uint8_t last, char *box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subcribers){
    struct Node *temp = (struct Node *)malloc(sizeof(struct Node));
    temp->last = last;
    temp->box_name = strdup(box_name);
    temp->box_size = box_size;
    temp->n_publishers = n_publishers;
    temp->n_subscribers = n_subcribers;
    temp->next = NULL;
    return temp;
}

void insertAlreadySorted(struct Node **head, uint8_t last, char *box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subcribers){
    struct Node *new_node = newNode(last, box_name, box_size, n_publishers, n_subcribers);
    struct Node *current;

    // case for beginning
    if (*head == NULL || strcmp((*head)->box_name, box_name) > 0) {
        new_node->next = *head;
        *head = new_node;
    } else {
        // place it already ordered or on the end
        current = *head;
        while (current->next != NULL &&
               strcmp(current->next->box_name, box_name) < 0) {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }
}

void printList(struct Node *node) {
    while (node != NULL) {
        fprintf(stdout, "%s %zu %zu %zu\n", node->box_name, node->box_size, node->n_publishers, node->n_subscribers);        
        node = node->next;
    }
}


    //FIXME verificar tamanho dos args


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
    
    uint8_t register_code;


    if (!strcmp(argv[3], CREATE_COMMAND)){
        register_code = CREATE_SEND_CODE;
    }
    else if (!strcmp(argv[3], REMOVE_COMMAND)){
        register_code = REMOVE_SEND_CODE;
    }
    else if (!strcmp(argv[3], LIST_COMMAND)){
        register_code = LIST_SEND_CODE;
    }
    else{
        PANIC("ERROR: MANAGER COMMAND");
    }

    
    //create message to send to server
        // message format: [ code = 3/5 ] | [ client_named_pipe_path (char[256])] | [ box_name (32)]
        // or [code = 7 ] | [ client_named_pipe_path (char[256])]

    void *register_message = malloc(sizeof(uint8_t) + MAX_PIPE_NAME + MAX_BOX_NAME);
    memset(register_message, 0, sizeof(uint8_t) + MAX_PIPE_NAME + MAX_BOX_NAME);

    memcpy(register_message, &register_code, sizeof(uint8_t));
    memcpy(register_message + sizeof(uint8_t), argv[2], strlen(argv[2]));
    
    if (register_code != LIST_SEND_CODE){
        memcpy(register_message + sizeof(uint8_t) + MAX_PIPE_NAME, argv[4], strlen(argv[4]));
    }
    
    assert(write(register_FIFO, register_message, sizeof(uint8_t) + MAX_PIPE_NAME + MAX_BOX_NAME) == sizeof(uint8_t) + MAX_PIPE_NAME + MAX_BOX_NAME);

    //opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    //QUESTIONS meto o fprintf?
    if (client_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    void *buffer = malloc(sizeof(uint8_t));

    
    //reads message
    //  message format: [ code = 4/6 ] | [ return_code (int32_t)] | [ message (char[1024]) ]
    //  or [ code = 8 ] | [ last (uint8_t)] | [ box_name char(32) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
    if(read(client_FIFO, buffer, sizeof(uint8_t)) == -1){
        PANIC("error reading from clients pipe");
    }

    uint8_t answer_code;
    memcpy(&answer_code, buffer, sizeof(uint8_t));
    switch (answer_code){
        case 4:
        case 6:
            uint32_t return_code;
            buffer = malloc(sizeof(int32_t) + MAX_MESSAGE);
            if(read(client_FIFO, buffer, sizeof(int32_t) + MAX_MESSAGE) == -1){
                PANIC("error readin from clients pipe");
            }

            memcpy(&return_code, buffer, sizeof(int32_t));
            if (return_code == 0){
                fprintf(stdout, "OK\n");
            }
            else {
                char *error = malloc(MAX_MESSAGE);
                memcpy(error, buffer + sizeof(int32_t), MAX_MESSAGE);
                fprintf(stdout, "ERROR %s\n", error);
            }
            break;
        case 8:
            buffer = malloc(sizeof(uint8_t) + MAX_BOX_NAME + 3 * sizeof(uint64_t));
            uint8_t last;
            char *box_name;
            box_name = malloc(MAX_BOX_NAME);
            uint64_t box_size, n_publishers, n_subscribers;

            memcpy(&last, buffer, sizeof(uint8_t));
            memcpy(box_name, buffer + sizeof(uint8_t), MAX_BOX_NAME);
            if (last == 1 && strlen(box_name) == 0){
                fprintf(stdout, "NO BOXES FOUND\n");
                break;
            }

            struct Node *head = NULL;
            
            while (last != 1){
                buffer = malloc(2 * sizeof(uint8_t) + MAX_MESSAGE + 3 * sizeof(uint64_t));
                memset(buffer, 0, sizeof(uint8_t) + MAX_MESSAGE + 3 * sizeof(uint64_t));
                ALWAYS_ASSERT(read(client_FIFO, buffer, 2 * sizeof(uint8_t) + MAX_MESSAGE + 3 * sizeof(uint64_t)) != -1, "manager: couldn't write into clients fifo");
                //tou bue atoa cm fazer ;7

                memcpy(&last, buffer + sizeof(uint8_t), sizeof(uint8_t));
                memcpy(box_name, buffer + 2*sizeof(uint8_t), MAX_BOX_NAME);
                memcpy(&box_size, buffer + 2*sizeof(uint8_t) + MAX_BOX_NAME, sizeof(uint64_t));
                memcpy(&n_publishers, buffer + 2*sizeof(uint8_t) + MAX_BOX_NAME +sizeof(uint64_t), sizeof(uint64_t));
                memcpy(&n_subscribers, buffer + 2*sizeof(uint8_t) + MAX_BOX_NAME + 2*sizeof(uint64_t), sizeof(uint64_t));
                
                insertAlreadySorted(&head, last, box_name, box_size, n_publishers, n_subscribers);
                memset(buffer, 0, 2 * sizeof(uint8_t) + MAX_MESSAGE + 3 * sizeof(uint64_t));

            }
            printList(head);

            break;
        default:
            PANIC("manager: code of command not found");
            break;
    }

    close(client_FIFO);
    close(register_FIFO);

    //remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    //TODO falta dar free

    return 0;
}
