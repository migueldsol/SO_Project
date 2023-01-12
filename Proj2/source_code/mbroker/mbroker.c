#include "logging.h"
#include "producer-consumer.h"
#include "betterassert.h"
#include "operations.h"
#include "mbroker.h"
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
        uint8_t code;
        memcpy(&code, elem, sizeof(int8_t));
        char *client_pipe_name, *box_name;
        client_pipe_name = malloc(MAX_CLIENT_PIPE);
        box_name = malloc(MAX_BOX_NAME);
        memset(client_pipe_name, 0, MAX_CLIENT_PIPE);
        memset(box_name, 0, MAX_BOX_NAME);
        memcpy(client_pipe_name, elem + sizeof(uint8_t), MAX_CLIENT_PIPE);
        memcpy(box_name, elem + sizeof(uint8_t) + MAX_CLIENT_PIPE, MAX_BOX_NAME);
        int client_fifo;

        switch (code){
            case 1:
                //pthread_rw_rdlock(&(important_values->boxes_lock));
                client_fifo = open(client_pipe_name, O_RDONLY);
                ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
                for (int i = 0; i < important_values->num_box; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0){
                        //pthread_rwlock_unlock(&(important_values->boxes_lock));
                        //pthread_mutex_lock(&(important_values->boxes[i].box_lock));

                        //FIXME if box nao foi apagada 

                        client_fifo = open(client_pipe_name, O_RDONLY);
                        ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");

                        char *pub_response = malloc(sizeof(char) * MAX_PUB_RESPONSE);
                        memset(pub_response, 0, MAX_PUB_RESPONSE);
                        int open_box = tfs_open(box_name, TFS_O_APPEND);
                        while(read(client_fifo, pub_response, MAX_PUB_RESPONSE) != 0){
                            //FIXMEif box nao foi apagada
                            char * message = malloc(sizeof(char) * MAX_PUB_MESSAGE);
                            memset(message, 0, MAX_PUB_MESSAGE);
                            memcpy(message, pub_response + sizeof(uint8_t),MAX_PUB_MESSAGE);
                            ssize_t size = tfs_write(open_box, message, MAX_PUB_MESSAGE);
                            important_values->boxes[i].box_size += (uint64_t)size;
                            memset(message, 0, MAX_PUB_MESSAGE);
                            memset(pub_response, 0, MAX_PUB_RESPONSE);
                        }
                        ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                    }
                } 
                ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                break;
            case 2:
                //pthread_rw_rdlock(&(important_values->boxes_lock));

                client_fifo = open(client_pipe_name, O_WRONLY);
                ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
                        
                for (int i = 0; i < important_values->num_box; i++){

                    if (strcmp(important_values->boxes[i].name, box_name) == 0){

                        //FIXME if box nao foi apagada 
                        
                        int open_box = tfs_open(box_name, 0);
                        char *message = malloc(MAX_PUB_MESSAGE);
                        char *buffer = malloc(sizeof(uint8_t) + MAX_PUB_MESSAGE);
                        uint8_t response_code = OP_CODE_SUB_RESPONSE;
                        while(true){
            
                            //FIXME if box nao foi apagada
                            //FIXME esperar que o pub escreva (com cond_wait)
                            memset(message, 0, MAX_PUB_MESSAGE);
                            memset(buffer, 0, sizeof(uint8_t) + MAX_PUB_MESSAGE);
                            ALWAYS_ASSERT(tfs_read(open_box, message, MAX_PUB_MESSAGE) != -1, "mbroker: Couldn't read the open box");
                            memcpy(buffer, &response_code, sizeof(uint8_t));
                            memcpy(buffer + sizeof(uint8_t), message, MAX_PUB_MESSAGE);

                            if(write(client_fifo, buffer, sizeof(uint8_t) + MAX_PUB_MESSAGE) == -1){
                                fprintf(stdout, "error in writing to clients fifo");
                                break;
                            }
                        }
                        ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                    }
                }
                ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the clients pipe");
                break;
            case 3:
                int leave = 0;
                client_fifo = open(client_pipe_name, O_WRONLY);
                for (int i = 0; i < important_values->num_box; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0){
                        leave = 1;
                    }
                }
                if (leave == 1){
                    //FIXME ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }

                int new_box = tfs_open(box_name, O_CREAT);
                if (new_box == -1){
                    break;
                }
        
                ALWAYS_ASSERT(tfs_close(new_box) != -1, "worker thread manager create error: couldn't close created box");
                break;
            case 5:
                client_fifo = open(client_pipe_name, O_WRONLY);

                for (int i = 0; i < important_values->num_box; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0 && code == 3){
                        
                    }
                }
                break;
            case 7://list boxes
                client_fifo = open(client_pipe_name, O_WRONLY);
                uint8_t code_8 = 8;
                if(client_fifo == -1){
                    printf("mbroker: Couldn't open the client's fifo");
                    break;
                }
                for (int i = 0; i < important_values->num_box; i++){
                    void *message = malloc(sizeof(char) * MAX_PUB_MESSAGE);
                    memset(message, 0, MAX_PUB_MESSAGE);
                    uint8_t last = 0;
                    if(i == important_values->num_box - 1){
                        last = 1;
                    }
                    //message with code=8(uint8_t)|last(uint8_t)|box_name(char[32])|box_size(uint64_t)|n_publishers(uint_64)|n_subs(uint64_t)|
                    memcpy(message, &code_8, sizeof(uint8_t));
                    memcpy(message, &last, sizeof(uint8_t));
                    memcpy(message, important_values->boxes[i].name, sizeof(char) * 32);
                    memcpy(message, &(important_values->boxes[i].box_size), sizeof(uint64_t));
                    memcpy(message, &(important_values->boxes[i].number_publishers), sizeof(uint64_t));
                    memcpy(message, &(important_values->boxes[i].number_subscribers), sizeof(uint64_t));

                    if (write(client_fifo, message, MAX_PUB_MESSAGE) == -1){
                        fprintf(stdout, "error in writing to clients fifo");
                        break;
                    }

                }
                break;
            default:
                PANIC("error in worker thread");
                break;

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

    size_t max_sessions;

    sscanf(argv[2], "%zu", &max_sessions);

    ALWAYS_ASSERT(pcq_create(queue, (size_t) max_sessions) == 0, "pqc_create fail");

    //initialize threads

    m_broker_values *important_values = malloc(sizeof(m_broker_values));
    
    box *boxes = malloc(default_params.max_inode_count * sizeof(box));
    important_values->num_box = 0;
    important_values->queue = queue;
    important_values->boxes = boxes; 
    pthread_rwlock_init(&(important_values->boxes_lock), NULL);

    pthread_t *threads = malloc(max_sessions * sizeof(pthread_t));

    for (int i = 0; i < max_sessions; i++){
        ALWAYS_ASSERT(pthread_create(&threads[i],NULL, &worker_thread, (void*)important_values) == 0, "Error creating threads");
    }

    if (mkfifo(argv[1], 0666) != 0){
        PANIC("error in creating the server fifi");
    }

    //this makes the mbroker never stop blocking on a read since there is someone already connected
    //  to the pipe as a writer

    int server = open(argv[1], O_RDONLY);
    ALWAYS_ASSERT(server != -1, "error in opening the register pipe");

    int afk_server = open(argv[1], O_WRONLY);
    ALWAYS_ASSERT(afk_server != -1, "error in opening the register pipe");

    ssize_t words = 0;
    while(true){
        void *buffer = malloc(sizeof(uint8_t) + MAX_CLIENT_PIPE + MAX_BOX_NAME);
        memset(buffer, 0, sizeof(uint8_t) + MAX_CLIENT_PIPE + MAX_BOX_NAME);
        words = read(server, buffer, sizeof(uint8_t) + MAX_CLIENT_PIPE + MAX_BOX_NAME);
        if (words == -1){
            PANIC("error reading from register_pipe");
        }
        else if (words == 0){
            PANIC("error, got an EOF");
        } 

        pcq_enqueue(queue, buffer);
    }

    close(afk_server);
    close(server);
    return 0;

}

