#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "types.h"
#include "task.h"
#include "signals.h"

int32_t syscall_isr(int32_t callnum, int32_t a, int32_t b, int32_t c);

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
int32_t _syscall_sigreturn(iret_context_t *context);
int32_t syscall_malloc(uint32_t size);
PCB_t *get_cur_pcb();
int32_t do_syscall(int32_t call, int32_t a, int32_t b, int32_t c);

#endif
