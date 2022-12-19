#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    //create new file 
    char *new_file_path = "/newFile";
    char link_name[] = "/link_ ";
    char *data = "good_info";
    char *previous_link_name = malloc(sizeof(char) * 7);
    assert(tfs_init(NULL) != -1);
    int fd = tfs_open(new_file_path, TFS_O_CREAT);
    tfs_write(fd, data, 9);
    assert(fd != -1);
    assert(tfs_close(fd)== 0);


    for (int i = 65; i < 75; i++){
        char index;
        index = (char)i;
        link_name[6] = index;
        if (i == 65){
            assert(tfs_sym_link(new_file_path, link_name)!= -1);
        }
        if (i != 65) {
            assert(tfs_sym_link(previous_link_name, link_name)!= -1);
        }
        memcpy(previous_link_name, link_name, sizeof(char) * 7);
    }
    char buffer[9];
    fd = tfs_open(link_name, TFS_O_CREAT);
    tfs_read(fd, buffer, 9);
    assert(memcmp(buffer,data,sizeof(char) * 9) == 0);
    assert(tfs_close(fd) == 0);
    printf("Successful test.\n");
    return 0;
}