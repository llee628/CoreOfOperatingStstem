#ifndef _TASK_H_
#define _TASK_H_

#include "types.h"

#define BUF_SIZE 256
// Maximum number of files open for each task
#define TASK_MAX_FILES 8
#define TASK_MAX_FD    7

#define TASK_IMG_START_ADDR 0x8048000

typedef struct {
    // Functions for operating on the file; in order of open, read, write, close
    void *file_ops;
    int32_t inode;
    int32_t pos;
    int32_t flags;
} FILE;

typedef struct PCB_s {
    FILE *open_files[TASK_MAX_FILES];
    struct PCB_s *parent;
    uint8_t signal;
} PCB_t;

int32_t load_task(uint8_t *filename, int32_t *entry_addr);

#endif /* ifndef _TASK_H_ */
