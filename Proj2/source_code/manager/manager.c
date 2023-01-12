#include "manager.h"
#include "variables.h"




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
        register_code = CREATE_BOX_CODE;
    }
    else if (!strcmp(argv[3], REMOVE_COMMAND)){
        register_code = REMOVE_BOX_CODE;
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

    void *register_message = malloc(MAX_SERVER_REGISTER);
    memset(register_message, 0, MAX_SERVER_REGISTER);

    memcpy(register_message, &register_code, UINT8_T_SIZE);
    memcpy(register_message + UINT8_T_SIZE, argv[2], strlen(argv[2]));
    
    if (register_code != LIST_SEND_CODE){
        memcpy(register_message + UINT8_T_SIZE + MAX_PIPE_NAME, argv[4], strlen(argv[4]));
    }
    
    assert(write(register_FIFO, register_message, MAX_SERVER_REGISTER) == MAX_SERVER_REGISTER);

    //opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    //QUESTIONS meto o fprintf?
    if (client_FIFO == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    void *buffer = malloc(UINT8_T_SIZE);

    
    //reads message
    //  message format: [ code = 4/6 ] | [ return_code (int32_t)] | [ message (char[1024]) ]
    //  or [ code = 8 ] | [ last (uint8_t)] | [ box_name char(32) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
    if(read(client_FIFO, buffer, UINT8_T_SIZE) == -1){
        PANIC("error reading from clients pipe");
    }

    uint8_t answer_code;
    memcpy(&answer_code, buffer, UINT8_T_SIZE);
    switch (answer_code){
        case 4:
        case 6:
            uint32_t return_code;
            buffer = malloc(INT32_T_SIZE + MAX_MESSAGE);
            if(read(client_FIFO, buffer, INT32_T_SIZE + MAX_MESSAGE) == -1){
                PANIC("error readin from clients pipe");
            }

            memcpy(&return_code, buffer, INT32_T_SIZE);
            if (return_code == 0){
                fprintf(stdout, "OK\n");
            }
            else {
                char *error = malloc(MAX_MESSAGE);
                memcpy(error, buffer + INT32_T_SIZE, MAX_MESSAGE);
                fprintf(stdout, "ERROR %s\n", error);
            }
            break;
        case 8:
            buffer = malloc(MAX_SERVER_BOX_LIST_REPLY);
            uint8_t last;
            char *box_name;
            box_name = malloc(MAX_BOX_NAME);
            uint64_t box_size, n_publishers, n_subscribers;

            memcpy(&last, buffer, UINT8_T_SIZE);
            memcpy(box_name, buffer + UINT8_T_SIZE, MAX_BOX_NAME);
            if (last == 1 && strlen(box_name) == 0){
                fprintf(stdout, "NO BOXES FOUND\n");
                break;
            }

            struct Node *head = NULL;
            
            while (last != 1){
                buffer = malloc(2 * UINT8_T_SIZE + MAX_MESSAGE + 3 * UINT64_T_SIZE);
                memset(buffer, 0, UINT8_T_SIZE + MAX_MESSAGE + 3 * UINT64_T_SIZE);
                ALWAYS_ASSERT(read(client_FIFO, buffer, 2 * UINT8_T_SIZE + MAX_MESSAGE + 3 * UINT64_T_SIZE) != -1, "manager: couldn't write into clients fifo");
                //tou bue atoa cm fazer ;7

                memcpy(&last, buffer + UINT8_T_SIZE, UINT8_T_SIZE);
                memcpy(box_name, buffer + 2*UINT8_T_SIZE, MAX_BOX_NAME);
                memcpy(&box_size, buffer + 2*UINT8_T_SIZE + MAX_BOX_NAME, UINT64_T_SIZE);
                memcpy(&n_publishers, buffer + 2*UINT8_T_SIZE + MAX_BOX_NAME +UINT64_T_SIZE, UINT64_T_SIZE);
                memcpy(&n_subscribers, buffer + 2*UINT8_T_SIZE + MAX_BOX_NAME + 2*UINT64_T_SIZE, UINT64_T_SIZE);
                
                insertAlreadySorted(&head, last, box_name, box_size, n_publishers, n_subscribers);
                memset(buffer, 0, 2 * UINT8_T_SIZE + MAX_MESSAGE + 3 * UINT64_T_SIZE);

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
