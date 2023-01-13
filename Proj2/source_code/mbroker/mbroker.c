#include "variables.h"
#include "producer-consumer.h"
#include "operations.h"
#include "mbroker.h"

int open_fifo( char *pipe_name, int flags){
    int client_fifo = open(pipe_name, flags);
    ALWAYS_ASSERT(client_fifo != -1, "mbroker: Couldn't open the client's fifo");
    return client_fifo;
}

void close_fifo(int client_fifo){
    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
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
/*
void pthread_m_lock_box(box *boxes) {
    pthread_mutex_t *mutex = get_mutex_table();
    ALWAYS_ASSERT(pthread_mutex_lock(&(boxes->box_lock)) == 0,
                  "pthread_mutex_lock: failed to lock");
}

void pthread_m_unlock_box(box *boxes) {
    pthread_mutex_t *mutex = get_mutex_table();
    ALWAYS_ASSERT(pthread_mutex_unlock(&(boxes->box_lock)) == 0,
                  "pthread_mutex_unlock: failed to unlock");
}
*/

int get_box_index(char *box_name, m_broker_values *broker){
    pthread_read_lock_broker(broker);
    int num_box = broker->num_box;
    for (int i = 0; i < num_box; i++) {
        if (strcmp(broker->boxes[i].name, box_name) == 0) {
            pthread_wr_unlock_broker(broker);
            return i;
        }
    }
    pthread_wr_unlock_broker(broker);
    return -1;
}

void *worker_thread(void * arg){
    m_broker_values *important_values = (m_broker_values*) arg;

    for(void *elem = pcq_dequeue(important_values->queue);1;elem = pcq_dequeue(important_values->queue)){
        uint8_t code;
        memcpy(&code, elem, UINT8_T_SIZE);

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
        int client_fifo;
        switch (code){
            case 1:
            //PUBLISHER
                //TODO colocar box locks
                int box_position = get_box_index(box_name, important_values);
                if (box_position == -1) {
                    //box nao existe
                    break;
                }
                client_fifo = open_fifo(client_pipe_name, O_RDONLY);
                char *pub_response = malloc(sizeof(char) * MAX_PUB_SUB_MESSAGE);
                memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
                int open_box = tfs_open(box_path, TFS_O_APPEND);

                //increment publishers in box
                important_values->boxes[box_position].number_publishers+= 1;

                //reading from publisher
                while(read(client_fifo, pub_response, MAX_PUB_SUB_MESSAGE) != 0){
                    //FIXMEif box nao foi apagada
                    char * message = malloc(sizeof(char) * MAX_MESSAGE);
                    memset(message, 0, MAX_MESSAGE);
                    memcpy(message, pub_response + UINT8_T_SIZE,MAX_MESSAGE);
                    ssize_t size = tfs_write(open_box, message, strlen(message) + 1);
                    //TODO verificar a escrita
                    important_values->boxes[box_position].box_size += (uint64_t)size;
                    memset(message, 0, MAX_MESSAGE);
                    memset(pub_response, 0, MAX_PUB_SUB_MESSAGE);
                }
                ALWAYS_ASSERT(tfs_close(open_box) != -1, "mbroker: Couldn't close tfs open box");
                close_fifo(client_fifo);
                break;
            case 2:
            //SUBSCRIBER
                client_fifo = open_fifo(client_pipe_name, O_WRONLY);
                //search for box
                int box_index = get_box_index(box_name, important_values);
                if (box_index == -1) {
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
                    ssize_t read_current;
                    //FIXME if box nao foi apagada
                    //FIXME esperar que o pub escreva (com cond_wait)
                    memset(message_to_send, 0, MAX_PUB_SUB_MESSAGE);
                    memcpy(message_to_send, &response_code, UINT8_T_SIZE);

                    //while read_current < size of box meter cond variable
                    read_current = tfs_read(open_box, message, MAX_MESSAGE);
                    ALWAYS_ASSERT(read_current != -1, "mbroker: Couldn't read the open box");

                    read_counter += read_current;
                    //TODO retirar espera ativa

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
                int leave = 0;
                client_fifo = open_fifo(client_pipe_name, O_WRONLY);
                void *manager_create_response = malloc(MAX_SERVER_REQUEST_REPLY);
                memset(manager_create_response, 0, MAX_SERVER_REQUEST_REPLY);
                uint8_t code_manager_create_reponse = CREATE_BOX_CODE_REPLY;
                int32_t return_code_create;
                memcpy(manager_create_response, &code_manager_create_reponse, UINT8_T_SIZE);
                //search for box with same name
                leave = get_box_index(box_name, important_values);
                //terminate
                if (leave != -1){
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
                    char error_message[] = "Error: box limit exceeded";
                    return_code_create = -1;
                    memcpy(manager_create_response + UINT8_T_SIZE, &return_code_create, INT32_T_SIZE);
                    memcpy(manager_create_response + UINT8_T_SIZE + INT32_T_SIZE, error_message, strlen(error_message));
                    ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                    ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                    break;
                }
                important_values->boxes[important_values->num_box].name = box_name;
                important_values->num_box++;
                return_code_create = 0;
                memcpy(manager_create_response + UINT8_T_SIZE, &return_code_create, INT32_T_SIZE);
                
                ALWAYS_ASSERT(write(client_fifo, manager_create_response, MAX_SERVER_REQUEST_REPLY) == MAX_SERVER_REQUEST_REPLY, "Couldn't write in clients fifo");
                ALWAYS_ASSERT(tfs_close(new_box) != -1, "worker thread manager create error: couldn't close created box");
                ALWAYS_ASSERT(close(client_fifo) != -1, "mbroker: Couldn't close the client's fifo");
                break;
            case 5:
            //MANAGER remove
                void *manager_remove_response = malloc(MAX_SERVER_REQUEST_REPLY);
                memset(manager_remove_response, 0, MAX_SERVER_REQUEST_REPLY);
                uint8_t manager_remove_reponse_code = REMOVE_BOX_CODE_REPLY;
                int32_t return_code_remove = 0;
                char *error_message = "";
                memcpy(manager_remove_response, &manager_remove_reponse_code, UINT8_T_SIZE);
                client_fifo = open(client_pipe_name, O_WRONLY);
                int box_num = get_box_index(box_name, important_values);
                if (box_num == -1){
                    error_message = "Error: box doesn't exist";
                    return_code_remove = -1;
                }
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
                void *message_list = malloc(MAX_PUB_SUB_MESSAGE);
                memset(message_list, 0, MAX_PUB_SUB_MESSAGE);
                uint8_t last = 0;
                if (important_values->num_box != 0){
                    for (int i = 0; i < important_values->num_box; i++){

                        if(i == important_values->num_box - 1){
                            last = 1;
                        }
                        //message with code=8(uint8_t)|last(uint8_t)|box_name(char[32])|box_size(uint64_t)|n_publishers(uint_64)|n_subs(uint64_t)|
                        memcpy(message_list, &code_8, UINT8_T_SIZE); 
                        memcpy(message_list + UINT8_T_SIZE, &last, UINT8_T_SIZE);
                        memcpy(message_list + 2* UINT8_T_SIZE, important_values->boxes[i].name, sizeof(char) * 32);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME, &(important_values->boxes[i].box_size), UINT64_T_SIZE);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME + UINT64_T_SIZE, &(important_values->boxes[i].number_publishers), UINT64_T_SIZE);
                        memcpy(message_list + 2 * UINT8_T_SIZE + MAX_BOX_NAME + 2 * UINT64_T_SIZE, &(important_values->boxes[i].number_subscribers), UINT64_T_SIZE);


                        ALWAYS_ASSERT(write(client_fifo, message_list, MAX_PUB_SUB_MESSAGE) != -1, "error in writing to clients fifo");
                        printf("wrote\n");
                    }
                }
                else {
                    last = 1;
                    memcpy(message_list, &code_8, UINT8_T_SIZE); 
                    memcpy(message_list + UINT8_T_SIZE, &last, UINT8_T_SIZE);
                    ALWAYS_ASSERT(write(client_fifo, message_list, MAX_PUB_SUB_MESSAGE) != -1, "error in writing to clients fifo");
                }
                ALWAYS_ASSERT(close(client_fifo) != -1, "couldn't close clients fifo");
                printf("left\n");
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
