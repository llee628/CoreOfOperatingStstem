#include "file_sys.h"
#include "lib.h"

static uint32_t bblock_ptr;
static inode_t* inodes;
static dentry_t* dentries;
static dentry_t cur_file;
static uint32_t cur_index;
static int num_read;


void fs_init(uint32_t boot_ptr){
    bblock_ptr = boot_ptr;
    dentries = (dentry_t*)(bblock_ptr + BBLOCK_DENTRIES_OFF);
    inodes = (inode_t*)(bblock_ptr + BLOCK_SIZE);
}

int fs_file_open(uint8_t* filename){
    num_read = 0;
    read_dentry_by_name(filename, &cur_file);
    return 0;
}

int fs_file_close(uint8_t* filename){
    int i;
    cur_file.filetype = -1;
    cur_file.inode_num = -1;
    num_read = 0;

    for(i = 0; i < MAX_NAME_LENGTH; i++)
      cur_file.filename[i] = '\0';

    return 0;
}


int fs_dir_open(uint8_t* filename){
    cur_index = 0;
    return 0;
}

int fs_dir_close(uint8_t * filename){
    cur_index = 0;
    return 0;
}

int fs_dir_read(uint8_t* buf){
    dentry_t read_dentry;
    read_dentry_by_index(cur_index, &read_dentry);
    strncpy((int8_t*)buf, (int8_t*)read_dentry.filename, MAX_NAME_LENGTH);
    cur_index++;
    return fn_length(buf);
}

int fs_file_read(uint8_t* buf, uint32_t length){
    printf("File size is %d\n", inodes[cur_file.inode_num].file_size);
    int bytes_read = (int)read_data(cur_file.inode_num, num_read, buf, length);
    num_read = (int)(num_read + bytes_read);
    return bytes_read;
}

int fs_dir_write(uint8_t* buf, uint32_t length){ return -1; }

int fs_file_write(uint8_t* buf, uint32_t length){ return -1; }

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    /* loop index */
    int i;
    /* temp name holder */
    int8_t cur_name[MAX_NAME_LENGTH];
    int32_t num_dentries = (int32_t)(*((uint32_t*)bblock_ptr));
    uint32_t name_length = fn_length(fname);

    /* loops through the dentries, skip the first one because that refers to the
      directory itself
    */
    for(i = 0; i < num_dentries; i++){
        /* set cur_name to the current dentry filename */
        strncpy(cur_name, (int8_t*)dentries[i].filename, name_length);
        /* check to see if the names are the same
          if they are then copy the relevant values to dentry */
        if(!strncmp(cur_name, (int8_t*)fname, name_length)){
          (void)read_dentry_by_index(i, dentry);
          return 0;
        }
    }

    return -1;
}


int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    /*local vars to hold the data we want to copy */
    int32_t file_type;
    int32_t inode_num;
    int32_t num_dentries = (int32_t)(bblock_ptr);
    /* check if the index is within acceptable bounds */
    if(index >= num_dentries || index < 0)
        return -1;
    /* get the relevant data to copy */
    file_type = (int32_t)(dentries[index].filetype);
    inode_num = (int32_t)(dentries[index].inode_num);

    //printf("File size from file sys is %d\n", inodes[(int32_t)(dentries[index].inode_num)].file_size );

    /* set dentry to be a copy of the corresponding file sys dentry */
    dentry->filetype = file_type;
    dentry->inode_num = inode_num;
    /* Names do not necessarily include a terminal EOS */
    strncpy((int8_t*)dentry->filename, (int8_t*)dentries[index].filename, MAX_NAME_LENGTH);

    return 0;
}


int32_t read_data(int32_t inode_num, uint32_t offset, uint8_t* buf, uint32_t length){
    /* for loop index */
    int i;
    /* local variables used to find byte data*/
    int32_t inode_count;
    int32_t data_block_count;
    uint32_t cur_data_block;
    int32_t data_offset;
    uint32_t cur_byte;
    int starting_index;
    int32_t actual_len;
    uint8_t* cur_buf_pos;
    int32_t num_bytes = 0;

    /* get the number of inodes */
    inode_count = (int32_t)(*((uint32_t*)(bblock_ptr + BBLOCK_COUNT_OFF)));

    /* checks to see if the inode_num is valid */
    if(inode_num > inode_count || inode_num < 0)
      return 0;

    /* get the number of data_blocks */
    data_block_count = (int32_t)(*((uint32_t*)(bblock_ptr + 2 * BOOT_ENTRIES_OFF)));

    /* checks to see if the entire file has already been read, return 0 if it has */
    if(offset >= inodes[inode_num].file_size)
      return 0;

    /* calculates the actual length to be read, truncates the length if it goes over the
      amount of bytes that have yet to be read */
    if( (length + offset > inodes[inode_num].file_size) )
      actual_len = inodes[inode_num].file_size - offset;
    else
      actual_len = length;

    /* store buf address with a temp variable that will be iterated through */
    cur_buf_pos = buf;

    /* calculates the index of which data block number to start on */
    starting_index = offset / BLOCK_SIZE;
    /* get the starting data block */
    cur_data_block = inodes[inode_num].data_blocks[starting_index];
    /* calculates where in the file to start reading from */
    data_offset = offset % BLOCK_SIZE;

    /* get the address of the first byte we want to read */
    cur_byte = bblock_ptr + inode_count * BLOCK_SIZE + cur_data_block * BLOCK_SIZE + data_offset + BLOCK_SIZE;

    /* loops through all the bytes to be read and copying them to the buffer */
    for(i = 0; i < actual_len; i++){
        /* checks to see if we should switch to another data block */
        if( !((i + data_offset) % BLOCK_SIZE) && (i + data_offset) ){
          starting_index++;
          cur_data_block = inodes[inode_num].data_blocks[starting_index];
          cur_byte = bblock_ptr + inode_count * BLOCK_SIZE + (cur_data_block) * BLOCK_SIZE + BLOCK_SIZE;
        }
        /* copy byte data to buffer */
        *cur_buf_pos = (int8_t)(*((uint32_t*)cur_byte));
        /* update the buffer to next position to fill */
        cur_buf_pos++;
        /* update byte address to the next byte to be read */
        cur_byte++;
        /* increment the number of bytes read counter */
        num_bytes++;
    }

    /* return the number of bytes read */
    return num_bytes;
}

uint32_t fn_length(const uint8_t* fname){
  uint32_t len = 0;
  while (fname[len] != '\0'){
      len++;
      if(len == MAX_NAME_LENGTH)
        break;
  }
  return len;
}

int32_t modified_puts(int8_t* s, uint32_t length){
    int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
				if(index == length)
					break;
    }
    return index;
}
