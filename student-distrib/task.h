#ifndef _TASK_H_
#define _TASK_H_

#include "types.h"
#include "page.h"

#define BUF_SIZE 256
// Maximum number of files open for each task
#define TASK_MAX_FILES 8
#define TASK_MAX_FD    7
// The starting page index for the first task = 8 MB
#define TASK_START_PAGE 2
#define TASK_PAGE_INDEX(c) (TASK_START_PAGE + c)

#define TASK_IMG_START_ADDR 0x08048000
// Kernel stack top for the task; Also location for PCB
#define TASK_KSTACK_TOP(c) (0x800000 - 0x2000 * (c + 1))
// Kernel stack bottom (start) for the task
#define TASK_KSTACK_BOT(c) (0x800000 - 0x2000 * (c))
// User stack bottom (start) for the task
#define TASK_USTACK_BOT(c) (0x800000 + 0x40000 * (c + 1))
#define KSTACK_TOP_MASK (~0x1FFF)

#define TASK_VIRT_PAGE_BEG (0x8000000)
#define TASK_VIRT_PAGE_END (0x8400000)

typedef struct {
    int32_t (*open)(const uint8_t* filename);
    int32_t (*read)(void* buf, int32_t nbytes);
    int32_t (*write)(const void* buf, int32_t nbytes);
    int32_t (*close)();
} file_ops_table_t;

typedef struct {
    // Functions for operating on the file; in order of open, read, write, close
    file_ops_table_t *file_ops;
    int32_t inode;
    int32_t pos;
    int32_t flags;
} FILE;

typedef struct PCB_s {
    FILE open_files[TASK_MAX_FILES];
    uint8_t open_file_count;
    struct PCB_s *parent;
    const uint8_t *cmd_args;
    uint8_t *prev_esp;
    uint8_t *prev_ebp;
    uint8_t *prev_eip;
    uint8_t signal;
    uint8_t pid;
} PCB_t;

int32_t load_task(uint8_t *filename, int32_t task_count, int32_t *entry_addr);

#endif /* ifndef _TASK_H_ */
