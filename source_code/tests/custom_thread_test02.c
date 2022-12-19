#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

char file_contents[] = "AAA";
char fp[] = "/f1";

int check_content(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    char buffer[strlen(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);
    assert(tfs_close(f) != -1);

    if (strcmp(buffer, "")==0 || strcmp(buffer, "AAA")==0|| strcmp(buffer, "AAAAAA")==0 || strcmp(buffer, "AAAAAAAAA")==0
    || strcmp(buffer, "AAAAAAAAAAAA")==0 || strcmp(buffer, "AAAAAAAAAAAAAAA")==0) {
        return 0;
    }
    return -1;
}

void write_contents(char const *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void *th_run01() {
    for (int i = 0; i < 10 ; i++) {
        assert(check_content(fp) == 0);
    }
    return 0;
}

void *th_run02() {
    for (int i = 0; i <5 ; i++) {
        write_contents(fp);
    }
    return 0;
}

void *th_run03() {
    for (int i = 0; i < 10 ; i++) {
        assert(check_content(fp) == 0);
    }
    return 0;
}

int main() {
    assert(tfs_init(NULL) != -1);
    int fd = tfs_open(fp, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    // Creates threads
    pthread_t t1, t2;

    // run1 - Writes in a file (in a loop)
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    // run2 - Reads the same file (in a loop)
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);

    // Joins both of them
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}