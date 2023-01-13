#include "variables.h"

// QUESTIONS uint_8

int main(int argc, char **argv) {
    assert(argc == 4);

    //TODO dar handle a fechar o pipe
    //TODO dar handle a caixa ser removida pelo manager
    //create FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    // check if the fifo was created
    if (mkfifo(argv[2], 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // open server FIFO
    int register_FIFO = open(argv[1], O_WRONLY);

    // QUESTIONS meto o fprintf?
    //  check if the fifo was opened
    if (register_FIFO == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // create message to send to server
    //  0001ola.fifo0\0\0\0\0....\0
    //  message format: [ code = 1 (uint8_t)] | [ client_named_pipe_path
    //  (char[256])] | [ box_name (32)]

    void *register_message =
        malloc(MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);
    uint8_t code = SUBSCRIBER_CODE;
    memset(register_message, 0, MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);
    memcpy(register_message, &code, UINT8_T_SIZE);
    memcpy(register_message + UINT8_T_SIZE, argv[2], strlen(argv[2]));
    memcpy(register_message + UINT8_T_SIZE + MAX_PIPE_NAME, argv[3],
           strlen(argv[3]));

    assert(write(register_FIFO, register_message,
                 MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE) ==
           MAX_BOX_NAME + MAX_PIPE_NAME + UINT8_T_SIZE);
    free(register_message);

    // opens clients FIFO
    int client_FIFO = open(argv[2], O_RDONLY);

    // QUESTIONS meto o fprintf?
    if (client_FIFO == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(MAX_PUB_SUB_MESSAGE);
    memset(buffer, 0, MAX_PUB_SUB_MESSAGE);

    
    //TODO dar handle a sigint
    //reads message
    //  message format: [ code = 10 ] | [ message (char[1024]) ]
    while(read(client_FIFO, buffer, MAX_PUB_SUB_MESSAGE) != 0){
        if (strlen(buffer + UINT8_T_SIZE) == 0){
            break;
        }
        fprintf(stdout, "%s\n", buffer + UINT8_T_SIZE);
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
