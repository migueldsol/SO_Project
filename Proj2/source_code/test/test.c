#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>   


#define FIFO_PATHNAME "ola.fifo"
#define FIFO_SERVER "server.fifo"
#define BOX_NAME "caixa_x"
#define MAX_SERVER_REGISTER (292)
#define MAX_MESSAGE (1024)
#define MAX_SERVER_MESSAGE (1028)


int main() {

    if (unlink(FIFO_SERVER) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", FIFO_SERVER,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(FIFO_SERVER, 0666) != 0){
        printf("erro\n");
        exit(EXIT_FAILURE);
    }

    int server = open(FIFO_SERVER, O_RDONLY);

    assert(server != 0);

    char buffer[MAX_SERVER_REGISTER];
    char buffer2[MAX_SERVER_MESSAGE];

    assert(read(server, buffer, MAX_SERVER_REGISTER) == MAX_SERVER_REGISTER);

    char *file_name = malloc(256);
    char *box = malloc(32);

    memcpy(file_name, buffer+4, 256);
    memcpy(box, buffer+260, 32);
    
    int pub_fifo = open(file_name, O_RDONLY);
    assert(pub_fifo != -1);

    assert(read(pub_fifo, buffer2, MAX_SERVER_MESSAGE) == MAX_SERVER_MESSAGE);

    char *message = malloc(MAX_MESSAGE);
    memset(message, 0, MAX_MESSAGE);

    memcpy(message, buffer2 + 4, MAX_MESSAGE);

    assert(read(server, buffer, MAX_SERVER_REGISTER) == MAX_SERVER_REGISTER);

    memset(file_name, 0, 256);
    memset(box, 0, 32);

    memcpy(file_name, buffer+4, 256);
    memcpy(box, buffer+260, 32);    

    int subs_fifo = open(file_name, O_WRONLY);
    assert(subs_fifo != -1);

    write(subs_fifo, message, MAX_MESSAGE);


    close(subs_fifo);
    close(pub_fifo);
    close(server);

    return 0;

}
