#include "variables.h"

int main(int argc, char **argv) {

    assert(argc == 4);
    //TODO dar handle a fechar o pipe
    //TODO dar handle a caixa ser removida pelo manager
    
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");

    // create FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
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
    //  message format: [ code = 1 ] | [ client_named_pipe_path (char[256])] | [
    //  box_name (32)]
    void *register_message = malloc(MAX_PUB_SUB_REQUEST);
    memset(register_message, 0, MAX_PUB_SUB_REQUEST);
    uint8_t register_code = PUBLISHER_CODE;
    memcpy(register_message, &register_code, UINT8_T_SIZE);
    memcpy(register_message + UINT8_T_SIZE, argv[2], strlen(argv[2]));
    memcpy(register_message + UINT8_T_SIZE + MAX_PIPE_NAME, argv[3],
           strlen(argv[3]));

    assert(write(register_FIFO, register_message,
                 UINT8_T_SIZE + MAX_PIPE_NAME + MAX_BOX_NAME) ==
           UINT8_T_SIZE + MAX_PIPE_NAME + MAX_BOX_NAME);
    free(register_message);
    // opens clients FIFO
    int client_FIFO = open(argv[2], O_WRONLY);

    // QUESTIONS meto o fprintf?
    // check if the client fifo was opened
    if (client_FIFO == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(MAX_MESSAGE);
    memset(buffer, 0, MAX_MESSAGE);

    char *server_command = malloc(MAX_PUB_SUB_MESSAGE);

    // QUESTIONS e so \n fazemos oq?
    // QUESTIONS se acabar em \0 termina?
    uint8_t publisher_message_code = PUBLISHER_MESSAGE_CODE;
    while (fgets(buffer, MAX_MESSAGE, stdin) != NULL) {
        if (*buffer == '\n'){
            break;
        }
        buffer = strtok(buffer, "\n");
        size_t length = strlen(buffer);
        if (length == 0) {
            break;
        }

        // setting up command to send to server
        //   command format: [ code = 10 ] | [ message (char[1024]) ]
        memset(server_command, 0, MAX_PUB_SUB_MESSAGE);
        memcpy(server_command, &publisher_message_code, UINT8_T_SIZE);
        memcpy(server_command + UINT8_T_SIZE, buffer, MAX_MESSAGE);

        ALWAYS_ASSERT(write(client_FIFO, server_command, MAX_PUB_SUB_MESSAGE) !=
                          -1,
                      "error writing message");
    }
    free(buffer);
    free(server_command);

    close(client_FIFO);
    close(register_FIFO);

    // remove client FIFO
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
