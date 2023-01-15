#include "manager.h"
#include "variables.h"

void free_linked_list(struct Node *head) {
    struct Node *current = head;
    struct Node *next;

    while (current != NULL) {
        next = current->next;
        free(current->box_name);
        free(current);
        current = next;
    }
}

struct Node *newNode(uint8_t last, char *box_name, uint64_t box_size,
                     uint64_t n_publishers, uint64_t n_subcribers) {
    struct Node *temp = (struct Node *)malloc(sizeof(struct Node));
    temp->last = last;
    temp->box_name = strdup(box_name);
    temp->box_size = box_size;
    temp->n_publishers = n_publishers;
    temp->n_subscribers = n_subcribers;
    temp->next = NULL;
    return temp;
}

void insertAlreadySorted(struct Node **head, uint8_t last, char *box_name,
                         uint64_t box_size, uint64_t n_publishers,
                         uint64_t n_subcribers) {
    struct Node *new_node =
        newNode(last, box_name, box_size, n_publishers, n_subcribers);
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
        fprintf(stdout, "%s %zu %zu %zu\n", node->box_name, node->box_size,
                node->n_publishers, node->n_subscribers);
        node = node->next;
    }
}

int main(int argc, char **argv) {
    assert(argc == 5 || argc == 4);

    // create FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(argv[2], 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // open server FIFO
    int register_FIFO = open(argv[1], O_WRONLY);

    // QUESTIONS meto o fprintf?
    if (register_FIFO == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    uint8_t register_code;

    if (!strcmp(argv[3], CREATE_COMMAND)) {
        register_code = CREATE_BOX_CODE;
    } else if (!strcmp(argv[3], REMOVE_COMMAND)) {
        register_code = REMOVE_BOX_CODE;
    } else if (!strcmp(argv[3], LIST_COMMAND)) {
        register_code = LIST_SEND_CODE;
    } else {
        PANIC("ERROR: MANAGER COMMAND");
    }

    // create message to send to server
    //  message format: [ code = 3/5 ] | [ client_named_pipe_path (char[256])] |
    //  [ box_name (32)] or [code = 7 ] | [ client_named_pipe_path (char[256])]

    void *register_message = malloc(MAX_SERVER_REGISTER);
    memset(register_message, 0, MAX_SERVER_REGISTER);

    memcpy(register_message, &register_code, UINT8_T_SIZE);
    memcpy(register_message + UINT8_T_SIZE, argv[2], strlen(argv[2]));

    if (register_code != LIST_SEND_CODE) {
        memcpy(register_message + UINT8_T_SIZE + MAX_PIPE_NAME, argv[4],
               strlen(argv[4]));
    }

    assert(write(register_FIFO, register_message, MAX_SERVER_REGISTER) ==
           MAX_SERVER_REGISTER);
    free(register_message);
    // opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    // QUESTIONS meto o fprintf?
    if (client_FIFO == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    void *buffer = malloc(UINT8_T_SIZE);
    memset(buffer, 0, UINT8_T_SIZE);
    // reads message
    //   message format: [ code = 4/6 ] | [ return_code (int32_t)] | [ message
    //   (char[1024]) ] or [ code = 8 ] | [ last (uint8_t)] | [ box_name
    //   char(32) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [
    //   n_subscribers (uint64_t) ]
    if (read(client_FIFO, buffer, UINT8_T_SIZE) == -1) {
        PANIC("error reading from clients pipe");
    }
    uint8_t answer_code;
    memcpy(&answer_code, buffer, UINT8_T_SIZE);
    switch (answer_code){
        case 4:
        case 6:
            uint32_t return_code;
            void *response_buffer = malloc(INT32_T_SIZE + MAX_MESSAGE);
            memset(response_buffer, 0, INT32_T_SIZE + MAX_MESSAGE);
            if(read(client_FIFO, response_buffer, INT32_T_SIZE + MAX_MESSAGE) == -1){
                PANIC("error readin from clients pipe");
            }

            memcpy(&return_code, response_buffer, INT32_T_SIZE);
            if (return_code == 0){
                fprintf(stdout, "OK\n");
            }
            else {
                char *error = malloc(MAX_MESSAGE);
                memcpy(error, response_buffer + INT32_T_SIZE, MAX_MESSAGE);
                fprintf(stdout, "ERROR %s\n", error);
            }
            free(response_buffer);
            break;

        case 8:
            uint8_t last;
            uint64_t box_size, n_publishers, n_subscribers;
            char *box_name = malloc(MAX_BOX_NAME);
            void *current_response = malloc(UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE);
            memset(current_response, 0, UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE);
            //we will handle the first message and verify if its the only one
            if(read(client_FIFO, current_response, UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE) == -1){
                PANIC("error readin from clients pipe");
            }
            memcpy(&last, current_response, UINT8_T_SIZE);
            memcpy(box_name, current_response + UINT8_T_SIZE, MAX_BOX_NAME);
            if (last == 1 && strlen(box_name) == 0){
                fprintf(stdout, "NO BOXES FOUND\n");
                free(current_response);
                break;
            }
            memcpy(&box_size, current_response + UINT8_T_SIZE + MAX_BOX_NAME, UINT64_T_SIZE);
            memcpy(&n_publishers, current_response + UINT8_T_SIZE + MAX_BOX_NAME +UINT64_T_SIZE, UINT64_T_SIZE);
            memcpy(&n_subscribers, current_response + UINT8_T_SIZE + MAX_BOX_NAME + 2*UINT64_T_SIZE, UINT64_T_SIZE);
            
            struct Node *head = NULL;
            insertAlreadySorted(&head, last, box_name, box_size, n_publishers, n_subscribers);
            free(current_response);
            //now checking if its the last message if not repeting the process
            while (last != 1){
                void *box_element = malloc(2 * UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE);
                memset(box_element, 0, 2 * UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE);
                ALWAYS_ASSERT(read(client_FIFO, box_element, 2 * UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE) != -1, "manager: couldn't write into clients fifo");
                memcpy(&last, box_element + UINT8_T_SIZE, UINT8_T_SIZE);
                memcpy(box_name, box_element + 2*UINT8_T_SIZE, MAX_BOX_NAME);
                memcpy(&box_size, box_element + 2*UINT8_T_SIZE + MAX_BOX_NAME, UINT64_T_SIZE);
                memcpy(&n_publishers, box_element + 2*UINT8_T_SIZE + MAX_BOX_NAME +UINT64_T_SIZE, UINT64_T_SIZE);
                memcpy(&n_subscribers, box_element + 2*UINT8_T_SIZE + MAX_BOX_NAME + 2*UINT64_T_SIZE, UINT64_T_SIZE);
                insertAlreadySorted(&head, last, box_name, box_size, n_publishers, n_subscribers);
                free(box_element);
            }
            printList(head);
            free_linked_list(head);
            break;

    default:
        PANIC("manager: code of command not found");
        break;
    }
    free(buffer);
    close(client_FIFO);
    close(register_FIFO);

    // remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    return 0;
}
