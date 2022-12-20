#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

char file_contents[] = "SO PROJECT!!!";
char text_path[] = "tests/customTEXT.txt";
char  f1[] = "/f1";
char  f2[] = "/f2";
char  f3[] = "/f3";
char  f4[] = "/f4";
char  f5[] = "/f5";
char  l1[] = "/l1";
char  l2[] = "/l2";
char  l3[] = "/l3";
char  l4[] = "/l4";
char  l5[] = "/l5";

void create(char path[]) {
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

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

    char buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, strlen(buffer)) == strlen(buffer));
    assert(memcmp(buffer, file_contents, strlen(buffer)) == 0);

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
    assert(tfs_link(f1, l1) != -1);
        tfs_copy_from_external_fs(text_path, l1);
        assert(tfs_unlink(f1) != -1);
        assert_contents_ok(l1);
        return 0;
}

void *th_run02() {
    assert(tfs_sym_link(f2, l2) != -1);
        assert_empty_file(l2);
        write_contents(l2);
        assert(tfs_unlink(f2) != -1);
        assert(tfs_open(l2, 0) == -1);
        create(f2);
        assert(tfs_open(l2, 0) != -1);
    
    return 0;
}

void *th_run03() {
    assert_empty_file(f3);
        assert(tfs_link(f3, l3) != -1);
        assert_empty_file(l3);
        assert(tfs_sym_link(f3, l4) != -1);
        assert_empty_file(l4);
        tfs_copy_from_external_fs(text_path, l4);
        tfs_unlink(f3);
        assert(tfs_open(l4, 0) == -1);
        assert_contents_ok(l3);
        return 0;
}

void *th_run04() {
    create(f4);
    assert(tfs_link(f4, l5) != -1);
    write_contents(f4);
    assert(tfs_unlink(f4) != -1);
    assert(tfs_open(f4, 0) == -1);
    assert(tfs_unlink(f4) == -1);
    assert_contents_ok(l5);
    assert(tfs_unlink(l5) != -1);
    assert(tfs_open(l5, 0) == -1);
    return 0;
}

void *th_run05() {
    create(f5);
    assert_empty_file(f5);
    write_contents(f5);
    assert_contents_ok(f5);
    assert(tfs_unlink(f5) != -1);
    assert(tfs_unlink(f5) == -1);
    assert(tfs_open(f5, 0) == -1);
    create(f5);
    assert(tfs_open(f5, 0) != -1);
    return 0;
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(f1);
    create(f2);
    create(f3);

    // Creates threads
    pthread_t /*t1,*/ t2, t3/*, t4, t5*/;

    // Every thread runs multiple instructions in a single file
    //assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);
    assert(pthread_create(&t3, NULL, th_run03, NULL) == 0);
    //assert(pthread_create(&t4, NULL, th_run04, NULL) == 0);
    //assert(pthread_create(&t5, NULL, th_run05, NULL) == 0);

    // Joins threads
    //assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);
    //assert(pthread_join(t4, NULL) == 0);
    //assert(pthread_join(t5, NULL) == 0);


    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
