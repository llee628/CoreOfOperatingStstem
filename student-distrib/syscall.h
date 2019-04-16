#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "types.h"
#include "task.h"
#include "signals.h"

// Each block has a size of 32-bytes, and the allocator will only allocate
// multiples of blocks
// Total amount of heap is 1MB, with a max block number of 2K (so total map
// size = 4K)
// A size of 0 means it has *1* block; used blocks can be ajacent to each
// other (as they represent deifferent allocated objects), but unused
// blocks can't. Each object can only have 1 block.
#define MALLOC_MAP_SIZE 4096
#define MALLOC_OBJ_NUM 2048
#define MALLOC_BLOCK_SIZE 32
#define MALLOC_HEAP_MAP_START (TASK_VIRT_PAGE_BEG + PCB_SIZE)
#define HEAP_START (TASK_VIRT_PAGE_BEG + PCB_SIZE + MALLOC_MAP_SIZE)
typedef struct {
    uint16_t used : 1;
    uint16_t size : 15;
} __attribute__((packed)) malloc_obj_t;

malloc_obj_t *malloc_objs = (malloc_obj_t *) MALLOC_HEAP_MAP_START;
// Total number of objects; unused objects are counted
uint32_t malloc_obj_count = 1;

int32_t syscall_halt(uint8_t status);
int32_t syscall_execute(const int8_t *command);
int32_t syscall_read(int32_t fd, void *buf, uint32_t nbytes);
int32_t syscall_write(int32_t fd, const void *buf, uint32_t nbytes);
int32_t syscall_open(const int8_t *filename);
int32_t syscall_close(int32_t fd);
int32_t syscall_getargs(int8_t *buf, uint32_t nbytes);
int32_t syscall_vidmap(uint8_t **screen_start);
int32_t syscall_set_handler(int32_t signum, void *handler);
extern int32_t syscall_sigreturn(void);
int32_t _syscall_sigreturn(hw_context_t *context);
int32_t syscall_malloc(uint32_t size);
PCB_t *get_cur_pcb();
int32_t do_syscall(int32_t call, int32_t a, int32_t b, int32_t c);

#endif
