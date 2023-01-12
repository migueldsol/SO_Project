#include "variables.h"
#include "producer-consumer.h"
#include "operations.h"
#include "mbroker.h"


    //FIXME verificar tamanho dos args

void *worker_thread(void * arg){
    m_broker_values *important_values = (m_broker_values*) arg;

    for(void *elem = pcq_dequeue(important_values->queue);1;elem = pcq_dequeue(important_values->queue)){
        uint8_t code;
        memcpy(&code, elem, UINT8_T_SIZE);

        char *client_pipe_name, *box_name;
        client_pipe_name = malloc(MAX_PIPE_NAME);
        box_name = malloc(MAX_BOX_NAME + 1);
        memset(client_pipe_name, 0, MAX_PIPE_NAME);
        memset(box_name, 0, MAX_BOX_NAME);
        memcpy(client_pipe_name, elem + UINT8_T_SIZE, MAX_PIPE_NAME);
        memcpy(box_name + 1, elem + UINT8_T_SIZE + MAX_PIPE_NAME, MAX_BOX_NAME);
        box_name[0] = '/';
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

                        char *pub_response = malloc(sizeof(char) * MAX_PUB_SUB_MESSAGE);
                        memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
                        int open_box = tfs_open(box_name, TFS_O_APPEND);
                        while(read(client_fifo, pub_response, MAX_PUB_SUB_MESSAGE) != 0){
                            //FIXMEif box nao foi apagada
                            char * message = malloc(sizeof(char) * MAX_MESSAGE);
                            memset(message, 0, MAX_MESSAGE);
                            memcpy(message, pub_response + UINT8_T_SIZE,MAX_MESSAGE);
                            ssize_t size = tfs_write(open_box, message, strlen(message) + 1);
                            //TODO verificar a escrita
                            important_values->boxes[i].box_size += (uint64_t)size;
                            memset(message, 0, MAX_MESSAGE);
                            memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
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
                        char *message = malloc(MAX_MESSAGE);
                        char *buffer = malloc(MAX_PUB_SUB_MESSAGE);
                        uint8_t response_code = SUBSCRIBER_MESSAGE_CODE;
                        while(true){
            
                            //FIXME if box nao foi apagada
                            //FIXME esperar que o pub escreva (com cond_wait)
                            memset(message, 0, MAX_MESSAGE);
                            memset(buffer, 0, MAX_PUB_SUB_MESSAGE);
                            ALWAYS_ASSERT(tfs_read(open_box, message, MAX_MESSAGE) != -1, "mbroker: Couldn't read the open box");
                            //TODO retirar espera ativa
                            memcpy(buffer, &response_code, UINT8_T_SIZE);
                            memcpy(buffer + UINT8_T_SIZE, message, MAX_MESSAGE);

                            if(write(client_fifo, buffer, MAX_PUB_SUB_MESSAGE) == -1){
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
                void *manager_create_response = malloc(MAX_SERVER_REQUEST_REPLY);
                memset(manager_create_response, 0, MAX_SERVER_REQUEST_REPLY);
                uint8_t code_manager_create_reponse = CREATE_BOX_CODE_REPLY;
                int32_t return_code;
                memcpy(manager_create_response, &code_manager_create_reponse, UINT8_T_SIZE);
                for (int i = 0; i < important_values->num_box; i++){
                    if (strcmp(important_values->boxes[i].name, box_name) == 0){
                        leave = 1;
                    }
                }
                if (leave == 1){
                    char error_message[] = "Error: box name already exists";
                    return_code = -1;
                    memcpy(manager_create_response + UINT8_T_SIZE, &return_code, INT32_T_SIZE);
                    memcpy(manager_create_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));

                    ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }

                int new_box = tfs_open(box_name, TFS_O_CREAT);
                if (new_box == -1){
                    char error_message[] = "Error: box limit exceeded";
                    return_code = -1;
                    memcpy(manager_create_response + UINT8_T_SIZE, &return_code, INT32_T_SIZE);
                    memcpy(manager_create_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));
                    ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }
                important_values->boxes[important_values->num_box].name = box_name;
                important_values->num_box++;
                return_code = 0;
                memcpy(manager_create_response + UINT8_T_SIZE, &return_code, INT32_T_SIZE);
                
                ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                ALWAYS_ASSERT(tfs_close(new_box) != -1, "worker thread manager create error: couldn't close created box");
                ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
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
                    void *message = malloc(sizeof(char) * MAX_PUB_SUB_MESSAGE);
                    memset(message, 0, MAX_PUB_SUB_MESSAGE);
                    uint8_t last = 0;
                    if(i == important_values->num_box - 1){
                        last = 1;
                    }
                    //message with code=8(uint8_t)|last(uint8_t)|box_name(char[32])|box_size(uint64_t)|n_publishers(uint_64)|n_subs(uint64_t)|
                    memcpy(message, &code_8, UINT8_T_SIZE);
                    memcpy(message, &last, UINT8_T_SIZE);
                    memcpy(message, important_values->boxes[i].name, sizeof(char) * 32);
                    memcpy(message, &(important_values->boxes[i].box_size), UINT64_T_SIZE);
                    memcpy(message, &(important_values->boxes[i].number_publishers), UINT64_T_SIZE);
                    memcpy(message, &(important_values->boxes[i].number_subscribers), UINT64_T_SIZE);

                    if (write(client_fifo, message, MAX_PUB_SUB_MESSAGE) == -1){
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
        //QUESTIONS temos que ler o tamanho certo de cada code?
        void *buffer = malloc(MAX_PUB_SUB_REQUEST);
        memset(buffer, 0, MAX_PUB_SUB_REQUEST);
        words = read(server, buffer, MAX_PUB_SUB_REQUEST);
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

