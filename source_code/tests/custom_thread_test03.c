#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

uint8_t  file_contents[] = "SO PROJECT!";
char  f1[] = "/f1";
char  f2[] = "/f2";
char  f3[] = "/f3";
char  f4[] = "/f4";
char  f5[] = "/f5";
char  l1[] = "/l1";
char  l2[] = "/l2";
char  l3[] = "/l3";
char  l4[] = "/l4";


void assert_empty_file(char  *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_contents_ok(char  *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char  *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void *th_run01() {
    assert(tfs_sym_link(f1, l1) != -1);
    write_contents(f1);
    assert_contents_ok(l1);
    assert(tfs_unlink(f1) != -1);
    assert(tfs_open(l1, 0) == -1);
    return 0;
}

void *th_run02() {
    assert_empty_file(f2);
    write_contents(f2);
    assert_contents_ok(f2);
    assert(tfs_unlink(f2) != -1);
    return 0;
}

void *th_run03() {
    assert_empty_file(f3);
    assert(tfs_link(f3, l2) != -1);
    assert(tfs_sym_link(f3, l3) != -1);
    assert_empty_file(l2);
    assert_empty_file(l3);
    write_contents(l2);
    assert(tfs_unlink(l2) != -1);
    assert_contents_ok(l3);
    return 0;
}

void *th_run04() {
    assert(tfs_link(f4, l4) != -1);
    assert_empty_file(f4);
    write_contents(f4);
    assert(tfs_unlink(f4) != -1);
    assert(tfs_unlink(f4) == -1);
    assert_contents_ok(l4);
    return 0;
}

void *th_run05() {
    assert_empty_file(f5);
    write_contents(f5);
    write_contents(f5);
    write_contents(f5);
    assert(tfs_unlink(f5) != -1);
    return 0;
}

void create(char path[]) {
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(f1);
    create(f2);
    create(f3);
    create(f4);
    create(f5);

    // Creates threads
    pthread_t t1, t2, t3, t4, t5;

    // Every thread runs multiple instructions in a single file
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);
    assert(pthread_create(&t3, NULL, th_run03, NULL) == 0);
    assert(pthread_create(&t4, NULL, th_run04, NULL) == 0);
    assert(pthread_create(&t5, NULL, th_run05, NULL) == 0);

    // Joins threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);
    assert(pthread_join(t4, NULL) == 0);
    assert(pthread_join(t5, NULL) == 0);


    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
