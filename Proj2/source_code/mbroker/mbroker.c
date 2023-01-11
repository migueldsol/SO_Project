#include "logging.h"
#include "producer-consumer.h"
#include "betterassert.h"
#include "operations.h"
#include "mbroker.h"
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
#include <pthread.h>
#define MAX_CLIENT_PIPE (256)
#define MAX_BOX_NAME (32)
#define MAX_PUB_RESPONSE (1028)
#define MAX_PUB_MESSAGE (1024)
#define MAX_SUB_RESPONSE (1028)
#define OP_CODE_SUB_RESPONSE (10)

    //FIXME verificar tamanho dos args

void *worker_thread(void * arg){
    m_broker_values *important_values = (m_broker_values*) arg;

    for(void *elem = pcq_dequeue(important_values->queue);1;elem = pcq_dequeue(important_values->queue)){
        int *code;
        memcpy(code, elem, sizeof(int) * 4);

        switch (*code){
            char *client_pipe_name, *box_name;
            memcpy(client_pipe_name, elem + 4, MAX_CLIENT_PIPE);
            memcpy(box_name, elem + 4 + MAX_CLIENT_PIPE, MAX_BOX_NAME);

            case 1:


                //pthread_rw_rdlock(&(important_values->boxes_lock));

                for (int i = 0; i < important_values->box_size; i++){
                    int cmp_value = strcmp(important_values->boxes[i].name, box_name);
                    if ((cmp_value == 0 && important_values->boxes->number_publishers == 1) || i == important_values->box_size - 1){
                        int client_fifo = open(client_pipe_name, O_RDONLY);
                        ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
                        ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                        //pthread_rwlock_unlock(&(important_values->boxes_lock));
                        break;
                    }
                    else if (cmp_value == 0){
                        //pthread_rwlock_unlock(&(important_values->boxes_lock));
                        //pthread_mutex_lock(&(important_values->boxes[i].box_lock));

                        //FIXME if box nao foi apagada 

                        int client_fifo = open(client_pipe_name, O_RDONLY);
                        ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");

                        char *pub_response = malloc(sizeof(char) * MAX_PUB_RESPONSE);
                        memset(pub_response, 0, MAX_PUB_RESPONSE);
                        int open_box = tfs_open(box_name, TFS_O_APPEND);
                        while(read(client_fifo, pub_response, MAX_PUB_RESPONSE) != 0){
                            //FIXMEif box nao foi apagada
                            char * message;
                            memcpy(message, pub_response + 4,MAX_PUB_MESSAGE);
                            ALWAYS_ASSERT(tfs_write(open_box, message, MAX_PUB_MESSAGE) != -1, "mbrocker: Couldn't write in box");
                            memset(message, 0, MAX_PUB_MESSAGE);
                            memset(pub_response, 0, MAX_PUB_RESPONSE);
                        }
                        ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                        ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the clients pipe");
                    }
                } 
                break;
            case 2:
                //pthread_rw_rdlock(&(important_values->boxes_lock));

                for (int i = 0; i < important_values->box_size; i++){
                    int cmp_value = strcmp(important_values->boxes[i].name, box_name);
                    if (cmp_value == 0){

                        //FIXME if box nao foi apagada 

                        int client_fifo = open(client_pipe_name, O_WRONLY);
                        ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
                        
                        int open_box = tfs_open(box_name, 0);
                        char *message = malloc(sizeof(char) * MAX_PUB_MESSAGE);
                        char *buffer = malloc(sizeof(char) * MAX_SUB_RESPONSE);
                        while(true){
                            //FIXME if box nao foi apagada
                            //FIXME esperar que o pub escreva (com cond_wait)
                            memset(message, 0, MAX_PUB_MESSAGE);
                            memset(buffer, 0, MAX_SUB_RESPONSE);
                            ALWAYS_ASSERT(tfs_read(open_box, message, MAX_PUB_MESSAGE) != -1, "mbroker: Couldn't read the open box");
                            sprintf(buffer, "%04d", OP_CODE_SUB_RESPONSE);
                            memcpy(buffer + 4, message, MAX_PUB_MESSAGE);
                            write(client_fifo, buffer, MAX_SUB_RESPONSE);
                        }
                        ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                        ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the clients pipe");
                    }
                    else if (i == important_values->box_size - 1){
                        int client_fifo = open(client_pipe_name, O_WRONLY);
                        ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
                        ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                        break;
                    }
                }
                break;
            case 3:

                int client_fifo = open(client_pipe_name, O_WRONLY);
                int exit = 0;
                for (int i = 0; i < important_values->box_size; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0){
                        exit = 1;
                    }
                }
                if (exit == 1){
                    //FIXME ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }

                int new_box = tfs_open(box_name, O_CREAT);
                if (new_box == -1){
                    break;
                }
        
                ALWAYS_ASSERT(tfs_close(new_box) != -1, "worker thread manager create error: couldn't close created box");

            case 5:
                int client_fifo = open(client_pipe_name, O_WRONLY);

                for (int i = 0; i < important_values->box_size; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0 && code == 3){
                        
                    }
                }

            case 7:

        }
    }
    return 0;
}

int main(int argc, char **argv) {

    assert(argc == 3);

    if (unlink(argv[1]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    //initialize tfs and boxes
    tfs_params default_params = tfs_default_params();

    ALWAYS_ASSERT(tfs_init(&default_params) == 0, "Error initiating tfs");

    //initialize producer-consumer queue
    pc_queue_t *queue = malloc(sizeof(pc_queue_t));

    ALWAYS_ASSERT(queue != NULL, "No memory for queue");

    unsigned long max_sessions = strtoull(argv[3], NULL, 10);

    ALWAYS_ASSERT(pcq_create(queue, (size_t) max_sessions) == 0, "pqc_create fail");

    //initialize threads

    m_broker_values *important_values = malloc(sizeof(m_broker_values));
    
    box *boxes = malloc(default_params.max_inode_count * sizeof(box)); 
    important_values->box_size = default_params.max_inode_count;
    important_values->queue = queue;
    important_values->boxes = boxes; 
    pthread_rwlock_init(&(important_values->boxes_lock), NULL);

    pthread_t *threads = malloc(max_sessions * sizeof(pthread_t));

    for (int i = 0; i < max_sessions; i++){
        ALWAYS_ASSERT(pthread_create(&threads[i],NULL, &worker_thread, (void*)important_values) == 0, "Error creating threads");
    }

    if (mkfifo(argv[1], 0666) != 0){
        printf("erro\n");
        exit(EXIT_FAILURE);
    }

    //this makes the mbroker never stop blocking on a read since there is someone already connected
    //  to the pipe as a writer
    int afk_server = open(argv[1], O_WRONLY);

    int server = open(argv[1], O_RDONLY);

    assert(server != 0);

    return 0;
}
