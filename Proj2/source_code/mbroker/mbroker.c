#include "variables.h"
#include "producer-consumer.h"
#include "operations.h"
#include "mbroker.h"

int fifo_leave = 0;

void remove_box_pub(int sig){
    if (sig == SIGUSR1){
        fifo_leave = 1;
    }
}


struct box *newBox(char *name){
    struct box* temp= (struct box*)malloc(sizeof(struct box));
    temp->name = strdup(name);
    temp->publisher_thread = (pthread_t *)malloc(sizeof(pthread_t));
    temp->number_publishers = 0;
    temp->number_subscribers = 0;
    temp->box_size = 0;
    pthread_cond_init(&temp->box_cond, NULL);
    pthread_mutex_init(&temp->box_lock, NULL);
    return temp;
}

void insertBox(m_broker_values *mbroker, char*name){
    pthread_write_lock_broker(mbroker);
    struct box *new_box = newBox(name);
    struct box *current;

    
    if (*mbroker->boxes_head == NULL){
        new_box->next = *mbroker->boxes_head;
        *mbroker->boxes_head = new_box;
    } else{
        current = *mbroker->boxes_head;

        while (current->next != NULL){
            current = current->next;
        }
        new_box->next = current->next;
        current->next = new_box;
    }
    mbroker->num_box++;
    pthread_wr_unlock_broker(mbroker);
}

struct box *getBox(m_broker_values *mbroker, char *name){
    pthread_read_lock_broker(mbroker);
    struct box *current = *mbroker->boxes_head;

    if (current == NULL){
        pthread_wr_unlock_broker(mbroker);
        return NULL;
    }

    while (current != NULL){
        if (strcmp(current->name, name) == 0){

            pthread_wr_unlock_broker(mbroker);
            return current;
        }
        current = current->next;
    }
    pthread_wr_unlock_broker(mbroker);
    return NULL;
}

struct box* deleteBox(m_broker_values *mbroker, char *name){
    pthread_write_lock_broker(mbroker);
    struct box *current = *mbroker->boxes_head;
    struct box *previous = NULL;

    if (current == NULL){
        pthread_wr_unlock_broker(mbroker);
        return NULL;
    }

    while (current->next != NULL){
        if (strcmp(current->name, name) == 0){
            previous->next = current->next;
            current->next = NULL;
            mbroker->num_box--;
            pthread_wr_unlock_broker(mbroker);
            return current;
        }
        previous = current;
        current = current->next;
    }
    pthread_wr_unlock_broker(mbroker);
    return NULL;
}

int open_fifo( char *pipe_name, int flags){
    int client_open_fifo = open(pipe_name, flags);
    ALWAYS_ASSERT(client_open_fifo != -1, "mbroker: Couldn't open the client's fifo");
    return client_open_fifo;
}

void close_fifo(int client_close_fifo){
    ALWAYS_ASSERT(close(client_close_fifo) != -1, "mbroker: Couldn't close the client's fifo");
}

void pthread_write_lock_broker(m_broker_values *broker) {
    ALWAYS_ASSERT(pthread_rwlock_wrlock(&(broker->boxes_lock)) == 0,
                  "pthread_write_lock: failed to lock");
}
void pthread_wr_unlock_broker(m_broker_values *broker) {
    ALWAYS_ASSERT(pthread_rwlock_unlock(&(broker->boxes_lock)) == 0,
                  "pthread_write_unlock: failed to unlock");
}
void pthread_read_lock_broker(m_broker_values *broker) {
    ALWAYS_ASSERT(pthread_rwlock_rdlock(&(broker->boxes_lock)) == 0,
                  "pthread_read_lock: failed to lock");
}

void pthread_box_broadcast(struct box *some_box){
    ALWAYS_ASSERT(pthread_cond_broadcast(&(some_box->box_cond)) == 0,
                "pthread_cond_broadcast: failed to broadcast");
}

void pthread_box_wait(struct box* some_box){
    ALWAYS_ASSERT(pthread_cond_wait(&some_box->box_cond, &some_box->box_lock) == 0,
                "pthread_cond_wait: failed to wait");
}

void pthread_box_lock(struct box *some_box){
    ALWAYS_ASSERT(pthread_mutex_lock(&some_box->box_lock) == 0,
                "pthread_mutex_lock: failed to lock");
}

void pthread_box_unlock(struct box *some_box){
    ALWAYS_ASSERT(pthread_mutex_unlock(&some_box->box_lock) == 0,
                "pthread_mutex_unlock: failed to unlock")
}

void *worker_thread(void * arg){
    m_broker_values *important_values = (m_broker_values*) arg;

    for(void *elem = pcq_dequeue(important_values->queue);1;elem = pcq_dequeue(important_values->queue)){
        uint8_t code;
        memcpy(&code, elem, UINT8_T_SIZE);
        int client_fifo;
        char *client_pipe_name, *box_name;
        client_pipe_name = malloc(MAX_PIPE_NAME);
        box_name = malloc(MAX_BOX_NAME);
        char *box_path = malloc(MAX_BOX_NAME + 1);

        memset(client_pipe_name, 0, MAX_PIPE_NAME);
        memset(box_name, 0, MAX_BOX_NAME);
        memset(box_path, 0 ,MAX_BOX_NAME + 1);

        memcpy(client_pipe_name, elem + UINT8_T_SIZE, MAX_PIPE_NAME);
        memcpy(box_name, elem + UINT8_T_SIZE + MAX_PIPE_NAME, MAX_BOX_NAME);
        memcpy(box_path + 1, box_name, strlen(box_name));

        box_path[0] = '/';

        switch (code){
            case 1:
            //PUBLISHER
                
                struct box *pub_box = getBox(important_values, box_name);

                if (pub_box == NULL) {
                    break;              //box doesn't exist
                }

                pthread_box_lock(pub_box);
                client_fifo = open_fifo(client_pipe_name, O_RDONLY);            //open fifo
                char *pub_response = malloc(sizeof(char) * MAX_PUB_SUB_MESSAGE);
                memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
                int open_box = tfs_open(box_path, TFS_O_APPEND);                //open box

                if (pub_box->number_publishers > 0){                            //check for another publisher
                    pthread_box_unlock(pub_box);
                    close_fifo(client_fifo);
                    break;
                }

                //increment publishers in box
                pub_box->number_publishers += 1;

                //associate publisher in box
                *pub_box->publisher_thread = pthread_self();

                pthread_box_unlock(pub_box);

                signal(SIGUSR1, remove_box_pub);            //handle removing the box
                //reading from publisher
                while(!fifo_leave){
                    ssize_t bytes_read = read(client_fifo, pub_response, MAX_PUB_SUB_MESSAGE);
                    if (bytes_read == 0 || (bytes_read == -1 && errno == EINTR)){
                        break;
                    }
                    else{
                        //FIXMEif box nao foi apagada
                        pthread_box_lock(pub_box);
                        char * message = malloc(sizeof(char) * MAX_MESSAGE);
                        memset(message, 0, MAX_MESSAGE);
                        memcpy(message, pub_response + UINT8_T_SIZE,MAX_MESSAGE);
                        ssize_t size = tfs_write(open_box, message, strlen(message) + 1);

                        if (size < strlen(message) + 1){
                            WARN("Message exceeded file limit");
                        }

                        pub_box->box_size += (uint64_t) size;
                        memset(message, 0, MAX_MESSAGE);
                        memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
                        pthread_box_unlock(pub_box);

                        pthread_box_broadcast(pub_box); 
                    }
                }
                
                pthread_box_lock(pub_box);
                pub_box->number_publishers -= 1;
                pthread_box_unlock(pub_box);
                close_fifo(client_fifo);
                fifo_leave = 0;
                break;
            case 2:
            //SUBSCRIBER
                client_fifo = open_fifo(client_pipe_name, O_WRONLY);
                //search for box
                struct box *subscriber_box = getBox(important_values, box_name);
                if (subscriber_box == NULL) {
                    //box nao existe
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the clients pipe");
                    break;

                }
                open_box = tfs_open(box_path, 0);
                ALWAYS_ASSERT(open_box != -1, "error in opening box");
                char *message = malloc(MAX_MESSAGE);
                char *message_to_send = malloc(MAX_PUB_SUB_MESSAGE);
                char *current_message = malloc(MAX_MESSAGE);
                uint8_t response_code = SUBSCRIBER_MESSAGE_CODE;

                //controlling how much of the file i've read
                ssize_t read_counter = 0;
                while(true){
                    //TODO se box existir
                    pthread_box_lock(subscriber_box);
                    if (read_counter == subscriber_box->box_size){
                        pthread_box_wait(subscriber_box);
                    }

                    if (subscriber_box->next != NULL && subscriber_box->next->name == NULL){
                        pthread_box_unlock(subscriber_box);
                        break;

                    }

                    pthread_box_unlock(subscriber_box);

                    ssize_t read_current;
                    memset(message_to_send, 0, MAX_PUB_SUB_MESSAGE);
                    memcpy(message_to_send, &response_code, UINT8_T_SIZE);

                    read_current = tfs_read(open_box, message, MAX_MESSAGE);
                    ALWAYS_ASSERT(read_current != -1, "mbroker: Couldn't read the open box");


                    read_counter += read_current;

                    size_t offset = 0;
                    size_t size = strlen(message);

                    while(offset != read_current && size != 0){
                        memcpy(current_message, message + offset, size + 1);

                        memcpy(message_to_send + UINT8_T_SIZE, current_message, MAX_MESSAGE);

                        if(write(client_fifo, message_to_send, MAX_PUB_SUB_MESSAGE) == -1){
                            fprintf(stdout, "error in writing to clients fifo");
                            break;
                        }
                        memset(current_message, 0, MAX_MESSAGE);
                        memset(message_to_send + UINT8_T_SIZE, 0, MAX_PUB_SUB_MESSAGE - UINT8_T_SIZE);
                        offset += size + 1;
                        size = strlen(message + offset);

                    }
                }
                ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                close_fifo(client_fifo);
                break;
            case 3:
            //MANAGER create
                struct box *manager_create_box;
                client_fifo = open_fifo(client_pipe_name, O_WRONLY);
                void *manager_create_response = malloc(MAX_SERVER_REQUEST_REPLY);
                memset(manager_create_response, 0, MAX_SERVER_REQUEST_REPLY);
                uint8_t code_manager_create_reponse = CREATE_BOX_CODE_REPLY;
                int32_t return_code_create;
                memcpy(manager_create_response, &code_manager_create_reponse, UINT8_T_SIZE);
                //search for box with same name
                manager_create_box = getBox(important_values, box_name);
                //terminate
                if (manager_create_box != NULL){
                    char error_message[] = "Error: box name already exists";
                    return_code_create = -1;
                    memcpy(manager_create_response + UINT8_T_SIZE, &return_code_create, INT32_T_SIZE);
                    memcpy(manager_create_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));

                    ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }

                int new_box = tfs_open(box_path, TFS_O_CREAT);
                //if cannot create box
                if (new_box == -1){
                    char error_message[] = "Error: couldn't create box in tfs";
                    return_code_create = -1;
                    memcpy(manager_create_response + UINT8_T_SIZE, &return_code_create, INT32_T_SIZE);
                    memcpy(manager_create_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));
                    ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }
                insertBox(important_values, box_name);


                return_code_create = 0;
                memcpy(manager_create_response + UINT8_T_SIZE, &return_code_create, INT32_T_SIZE);
                
                ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                ALWAYS_ASSERT(tfs_close(new_box) != -1, "worker thread manager create error: couldn't close created box");
                ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                break;
            case 5:
            //MANAGER remove
            //TODO remover box
                void *manager_remove_response = malloc(MAX_SERVER_REQUEST_REPLY);
                memset(manager_remove_response, 0, MAX_SERVER_REQUEST_REPLY);
                uint8_t manager_remove_reponse_code = REMOVE_BOX_CODE_REPLY;
                int32_t return_code_remove = 0;
                char *error_message = "";
                memcpy(manager_remove_response, &manager_remove_reponse_code, UINT8_T_SIZE);
                client_fifo = open(client_pipe_name, O_WRONLY);
                struct box *manager_remove_box = getBox(important_values, box_name);
                if (manager_remove_box == NULL){
                    error_message = "Error: box doesn't exist";
                    return_code_remove = -1;
                }

                pthread_box_lock(manager_remove_box);
                if (manager_remove_box->number_publishers == 1){
                    pthread_kill(*manager_remove_box->publisher_thread, SIGUSR1);
                }
                struct box *null_box = newBox(NULL);
                manager_remove_box->next = null_box;
                pthread_box_unlock(manager_remove_box);

                pthread_box_broadcast(manager_remove_box);                      //tell the subscribers the box is closed
                ALWAYS_ASSERT(tfs_unlink(box_path) != -1, "Couldn't unlink box");
                memcpy(manager_remove_response + UINT8_T_SIZE, &return_code_remove, INT32_T_SIZE);
                memcpy(manager_remove_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));
                ALWAYS_ASSERT(write(client_fifo, manager_remove_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Error in writting clients fifo");
                break;
            case 7:
            //MANAGER list
                client_fifo = open(client_pipe_name, O_WRONLY);
                ALWAYS_ASSERT(client_fifo != -1, "couldn't open pipe");
                uint8_t code_8 = LIST_RECEIVE_CODE;
                if(client_fifo == -1){
                    printf("mbroker: Couldn't open the client's fifo");
                    break;
                }
                void *message_list = malloc(MAX_SERVER_BOX_LIST_REPLY);
                memset(message_list, 0, MAX_SERVER_BOX_LIST_REPLY);
                uint8_t last = 0;
                if (important_values->num_box != 0){
                    pthread_read_lock_broker(important_values);
                    struct box *current = *important_values->boxes_head;
                    while (current != NULL){
                        
                        if(current->next == NULL){
                            last = 1;
                        }
                        //message with code=8(uint8_t)|last(uint8_t)|box_name(char[32])|box_size(uint64_t)|n_publishers(uint_64)|n_subs(uint64_t)|
                        memcpy(message_list, &code_8, UINT8_T_SIZE); 
                        memcpy(message_list + UINT8_T_SIZE, &last, UINT8_T_SIZE);
                        memcpy(message_list + 2* UINT8_T_SIZE, current->name, sizeof(char) * 32);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME, &(current->box_size), UINT64_T_SIZE);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME + UINT64_T_SIZE, &(current->number_publishers), UINT64_T_SIZE);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME + 2 * UINT64_T_SIZE, &(current->number_subscribers), UINT64_T_SIZE);


                        ALWAYS_ASSERT(write(client_fifo, message_list, MAX_SERVER_BOX_LIST_REPLY) != -1, "error in writing to clients fifo");
                        current = current->next;
                    }
                    pthread_wr_unlock_broker(important_values);
                }
                else {
                    last = 1;
                    memcpy(message_list, &code_8, UINT8_T_SIZE); 
                    memcpy(message_list + UINT8_T_SIZE, &last, UINT8_T_SIZE);
                    ALWAYS_ASSERT(write(client_fifo, message_list, MAX_SERVER_BOX_LIST_REPLY) != -1, "error in writing to clients fifo");
                }
                ALWAYS_ASSERT(close(client_fifo) != -1, "couldn't close clients fifo");
                break;
            default:
                PANIC("error in worker thread");
                break;
        }
    }
    return 0;
}

void sigint_handler(int sig) {

    if(sig == SIGINT){
        return;
    }
}

int main(int argc, char **argv) {

    assert(argc == 3);

    signal(SIGINT, sigint_handler);

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
    
    important_values->num_box = 0;
    important_values->queue = queue;
    important_values->boxes_head = malloc(sizeof(struct box*));
    *important_values->boxes_head = NULL; 
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

    pause();

    if (unlink(argv[1]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(afk_server);
    close(server);
    return 0;

}