#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betterassert.h"

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    /*
    int i_number = find_in_dir(root_inode, "");
    ALWAYS_ASSERT(i_number == ROOT_DIR_INUM, "tfs_lookup: root_inode should be the root");
    */
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;
    inode_t *inode; 
    if (inum >= 0){
        inode = inode_get(inum);
        
        // check if the link is a soft_link
        if(inode->i_node_type == T_SYMLINK){
            ALWAYS_ASSERT(inode != NULL,
                    "tfs_open: directory files must have an inode");
            char *buffer = data_block_get(inode->i_data_block);
            inum = tfs_lookup(buffer, root_dir_inode);
            if (inum < 0){
                return -1;
            }
            // get the inum of the file
        }
        // The file already exists
        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

inode_t* tfs_open_sym_link(int inum, inode_t *root_dir_inode){
    inode_t *inode = inode_get(inum);
    if(inode->i_node_type == T_SYMLINK){
        ALWAYS_ASSERT(inode != NULL,
                "tfs_open: directory files must have an inode");
        char buffer[inode->i_size];
        memcpy(buffer, data_block_get(inode->i_data_block), inode->i_size);
        inum = tfs_lookup(buffer, root_dir_inode);
        if (inum >= 0){
            return inode_get(inum);
        }
    }
    return NULL;
}

int tfs_sym_link(char const *target, char const *link_name) {
    inode_t *root_inode = inode_get(ROOT_DIR_INUM);
    int i_number_softlink = inode_create(T_SYMLINK);
    if (i_number_softlink == -1){
        return -1;
    }
    if (add_dir_entry(root_inode, link_name + 1, i_number_softlink) == -1){
        return -1;
    }
    inode_t *soft_link = inode_get(i_number_softlink);
    if (soft_link == NULL){
        return -1;
    }
    soft_link->i_size = strlen(target);
    soft_link->i_data_block = data_block_alloc();
    if (soft_link->i_data_block == -1){
        return -1;
    }
    void *write = data_block_get( soft_link->i_data_block);
    memcpy(write, target, strlen(target));
    soft_link->hard_link = -1;
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    
    inode_t *root_inode = inode_get(ROOT_DIR_INUM);
    int target_i_number = tfs_lookup(target, root_inode);
    if (target_i_number != -1){
        inode_t *inode = inode_get(target_i_number);
        if (inode->i_node_type == T_SYMLINK || add_dir_entry(root_inode, link_name + 1, target_i_number) == -1){
            return -1;
        }
        
        inode->hard_link++;
        return 0;
    }
    return -1;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    inode_t *root_inode = inode_get(ROOT_DIR_INUM);
    int i_number = find_in_dir(root_inode, target+1);
    if (i_number == -1){
        return -1;
    }
    inode_t *link_inode = inode_get(i_number);
    inode_type i_type = link_inode->i_node_type;
    
    ALWAYS_ASSERT(clear_dir_entry(root_inode, target+1) == 0, "tfs_unlink : couldn clear dir entry");
    switch (i_type){
        case T_FILE: {
            if (link_inode->hard_link != 1){
                link_inode->hard_link--;
            }
            else if (link_inode->hard_link == 1) {
                inode_delete(i_number);
            }
        }break;
        case T_SYMLINK: {
            inode_delete(i_number);
        }break;
        case T_DIRECTORY: {
            ALWAYS_ASSERT(false, "tfs_unlink: cannot unlink directory");
            return -1;
        }break;
        default: {
            PANIC("tfs_unlink: unknown inode type");
        }
    }
    return 0;
}

void close_files(FILE *source_path, int dest_path){
    fclose(source_path);
    tfs_close(dest_path);
}
int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    FILE *source = fopen(source_path,"r");
    int location = tfs_open(dest_path,TFS_O_CREAT);
    if(location == -1 || source == NULL){return -1;}
    char buffer[128];
    int counter = 0;
    while(counter < 7){
        memset(buffer,0,sizeof(buffer));
        size_t words = fread(buffer,sizeof(char),sizeof(buffer),source);
        if (ferror(source) != 0){
            close_files(source,location);
            return -1;
        }
        tfs_write(location,buffer,words);
        counter++;
    }
    close_files(source,location);
    return 0;
    
}
