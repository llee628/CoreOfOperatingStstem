#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"

#define MAX_NAME_LENGTH           32
#define MAX_BLOCK_NUM           1023
#define MAX_FILE_NUM              63
#define BLOCK_SIZE            0x1000
#define BBLOCK_DENTRIES_OFF       64
#define BBLOCK_COUNT_OFF           4
#define INODE_NUM_OFF             36
#define REGULAR_FILE               2
#define BOOT_ENTRIES_OFF           4
#define INODE_ENTRIES_OFF          4

/* data structures are based off of those discussed in lecture 16 */

typedef struct dentry{
    uint8_t filename[MAX_NAME_LENGTH];
    int32_t filetype;
    int32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

typedef struct inode{
    uint32_t file_size;
    uint32_t data_blocks[MAX_BLOCK_NUM];
} inode_t;

void fs_init(uint32_t boot_ptr);

int fs_file_open(uint8_t* filename);

int fs_file_close(uint8_t* filename);

int fs_dir_open(uint8_t* filename);

int fs_dir_close(uint8_t * filename);

int fs_dir_read(uint8_t* buf);

int fs_file_read(uint8_t* buf, uint32_t length);

int fs_dir_write(uint8_t* buf, uint32_t length);

int fs_file_write(uint8_t* buf, uint32_t length);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

int32_t read_data(int32_t inode_num, uint32_t offset, uint8_t* buf, uint32_t length);

uint32_t fn_length(const uint8_t* fname);

int32_t modified_puts(int8_t* s, uint32_t length);

int32_t get_type();

int32_t get_size();

#endif
