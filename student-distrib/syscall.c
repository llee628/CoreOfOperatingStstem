#include "syscall.h"
#include "term.h"
#include "lib.h"
#include "task.h"
#include "rtc.h"
#include "file_sys.h"
#include "x86_desc.h"

static int task_count = 1;

int32_t syscall_isr(int32_t callnum, int32_t a, int32_t b, int32_t c) {
    switch (callnum) {
        case 1: return syscall_halt((uint8_t) a);
        case 2: return syscall_execute((const uint8_t *) a);
        case 3: return syscall_read(a, (void *) b, c);
        case 4: return syscall_write(a, (void *) b, c);
        case 5: return syscall_open((const uint8_t *) a);
        case 6: return syscall_close(a);
        case 7: return syscall_getargs((uint8_t *) a, b);
        case 8: return syscall_vidmap((uint8_t **) a);
        case 9: return syscall_set_handler(a, (void *) b);
        case 10: return syscall_sigreturn();
        default: return -1;
    }
}

int32_t syscall_halt(uint8_t status) {
    // Revert info from PCB
    uint32_t cur_esp;
    asm volatile ("movl %%esp, %0;" : "=r" (cur_esp));
    PCB_t *task_pcb = (PCB_t *) (cur_esp & KSTACK_TOP_MASK);
    // TODO close files
    PCB_t *parent_pcb = task_pcb->parent;
    if (parent_pcb) {
        int32_t ppid = parent_pcb->pid;
        page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(ppid);
        tss.esp0 = TASK_KSTACK_BOT(ppid);
        // Reload the TLB
        asm volatile(
            "movl %%cr3, %%eax;"
            " movl %0, %%cr3; "
            :
            : "r"(page_directory)
            :"%eax"
        );
        //ltr(KERNEL_TSS);
    }
    uint32_t prev_ebp = (uint32_t) task_pcb->prev_ebp;
    uint32_t prev_esp = (uint32_t) task_pcb->prev_esp;
    asm volatile (
        "xorl %%ebx, %%ebx;"
        "movb %0, %%bl;"
        "movl %1, %%eax;"
        "movl %2, %%ecx;"
        "movl %%eax, %%ebp;"
        "movl %%ecx, %%esp;"
        "jmp syscall_execute__done;"
        :
        : "m" (status), "m" (prev_ebp), "m" (prev_esp)
        : "%eax", "%ecx", "%ebx"
    );
    // unreachable!!!
    while (1) { asm volatile ("hlt;"); }
    return 0;
}

int32_t syscall_execute(const uint8_t* command) {
    int i;
    for (i = 0; isalnum(command[i]); i ++);
    const uint8_t *args = command + i;

    // 1. Setup arguments
    if (*args) {
        while (*args == ' ') {
            args ++;
        }
    } else {
        args = NULL;
    }

    // 2. Copy the command itself so we have a guaranteed null-terminated string
    uint8_t filename[i + 1];
    strncpy((int8_t *) filename, (const int8_t *) command, i + 1);
    filename[i] = 0;

    int32_t entry_addr;
    // 3 & 4. Load task and setup paging
    if (load_task(filename, task_count, &entry_addr) == -1) {
        return -1;
    }

    // 5. Setup PCB
    PCB_t *task_pcb = (PCB_t *) TASK_KSTACK_TOP(task_count);
    // TODO Open files
    task_pcb->open_file_count = 2;
    task_pcb->cmd_args = args;
    task_pcb->signal = 0;
    asm volatile ("movl %%ebp, %0;" : "=r" (task_pcb->prev_ebp));
    asm volatile ("movl %%esp, %0;" : "=r" (task_pcb->prev_esp));
    PCB_t *parent_pcb = (PCB_t *) ((uint32_t) task_pcb->prev_esp & (uint32_t) KSTACK_TOP_MASK);
    if (parent_pcb != (PCB_t *) TASK_KSTACK_TOP(0)) {
        task_pcb->parent = parent_pcb;
    } else {
        task_pcb->parent = NULL;
    }
    task_pcb->prev_eip = 0;     // TODO confirm the label works
    task_pcb->pid = task_count;

    tss.esp0 = TASK_KSTACK_BOT(task_count);
    tss.ss0 = KERNEL_DS;
    //ltr(KERNEL_TSS);        // TODO how to reload Task Register?

    task_count ++;

    // 6. Context switch
    asm volatile (
        "movl %0, %%eax;"  // User CS
        "movw %%ax, %%ds;"
        "movw %%ax, %%es;"
        "movw %%ax, %%fs;"
        "movw %%ax, %%gs;"
        "push %%eax;"
        "movl %1, %%eax;" // User ESP
        "push %%eax;"
        "pushf;"
        /* "orl $0x200, (%%esp);"      // Enable interrupt on the flags */
        "movl %2, %%eax;" // User DS
        "push %%eax;"
        "movl %3, %%eax;" // Entry point
        "push %%eax;"
        "iret;"
"syscall_execute__done:;"
        :
        : "r" (USER_DS), "r" (TASK_VIRT_PAGE_END), "r" (USER_CS), "r" (entry_addr)
        : "%eax"
    );

    int32_t retval;
    asm volatile ("movl %%ebx, %0;" : "=r" (retval));
    return retval;
}

int32_t syscall_read(int32_t fd, void *buf, int32_t nbytes) {
    if (fd == 0) {
        return term_read(buf, nbytes);
    }

    if (fd == 1) {
        return -1;
    }
    return 0;
}

int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {
    if (fd == 0) {
        return -1;
    }

    if (fd == 1) {
        return term_write(buf, nbytes);
    }
    return 0;
}

/* syscall_open
 *  Descrption: Add file descriptor on a process's PCB
 *  
 *  Arg:
 *      filename: name of the target file
 * 
 * 	RETURN:
 *      file descriptor number, which is a small non-negative number,
 *      -1 if failed.
 */
int32_t syscall_open(const uint8_t* filename) {
    const int8_t * rtc_filename = "/rtc";

    // Copy the command itself so we have a guaranteed null-terminated string
    int i;
    for (i = 0; isalnum(filename[i]); i ++);
    uint8_t fname[i + 1];
    strncpy((int8_t *) fname, (const int8_t *) filename, i + 1);
    fname[i] = 0;

    // get current pcb
    PCB_t *task_pcb = (PCB_t *) TASK_KSTACK_TOP(task_count);
    if( task_pcb->open_file_count >= TASK_MAX_FILES ){
        return -1;
    }
    uint8_t fd = task_pcb->open_file_count++;

    //set up file descriptor
    {
        uint32_t flen = strlen((const int8_t*)fname);
        if( flen==strlen(rtc_filename) &&  strncmp((const int8_t*)fname,rtc_filename, flen)==0){
            task_pcb->open_files[fd].file_ops = &rtc_file_ops_table;
            //TODO: virtualize RTC and set the RTC info into PCB

        }else{
            if (fs_file_open(fname) == -1){
                return -1;
            }else{
                //TODO: add file operation function table here
            }
        }
    }
    return fd;
}

int32_t syscall_close(int32_t fd) {
    return 0;
}

int32_t syscall_getargs(uint8_t* buf, int32_t nbytes) {
    return 0;
}

int32_t syscall_vidmap(uint8_t** screen_start) {
    return 0;
}

int32_t syscall_set_handler(int32_t signum, void* handler) {
    return 0;
}

int32_t syscall_sigreturn(void) {
    return 0;
}
