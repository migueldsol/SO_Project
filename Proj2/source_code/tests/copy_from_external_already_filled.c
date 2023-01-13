#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    // tests if the copy_from_external clears the file before writing

    char *str_new_file =
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB!";
    unsigned long new_file_len = strlen(str_new_file);
    char *str_to_compare = "BBB!";
    unsigned long compare_len = strlen(str_to_compare);
    char *new_file_path = "/newFile";
    char *path_src = "tests/file_to_copy.txt";
    char buffer[compare_len];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    int fd = tfs_open(new_file_path,
                      TFS_O_CREAT); // creates a new file nammed newFile
    assert(fd != -1);

    assert(tfs_write(fd, str_new_file, new_file_len) !=
           -1); // writes big text in newFile

    f = tfs_copy_from_external_fs(
        path_src,
        new_file_path); // copies small text from path_src to new_file_path
    assert(f != -1);

    f = tfs_open(new_file_path, TFS_O_CREAT); // opens the copied file
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer)); // copies the text inside the file
    assert(r == compare_len);
    assert(!memcmp(buffer, str_to_compare,
                   compare_len)); // verifies that the small text is there

    printf("Successful test.\n");

    return 0;
}