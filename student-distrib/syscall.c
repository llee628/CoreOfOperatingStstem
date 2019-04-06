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
        case 2: return syscall_execute((const int8_t *) a);
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
    PCB_t *task_pcb = get_cur_pcb();
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
        "jmp 1f;"
        :
        : "m" (status), "m" (prev_ebp), "m" (prev_esp)
        : "%eax", "%ecx", "%ebx"
    );
    // unreachable!!!
    while (1) { asm volatile ("hlt;"); }
    return 0;
}

int32_t syscall_execute(const int8_t* command) {
    int i;
    for (i = 0; isalnum(command[i]); i ++);
    const int8_t *args = command + i;

    // 1. Setup arguments
    if (*args) {
        while (*args == ' ') {
            args ++;
        }
    } else {
        args = NULL;
    }

    // 2. Copy the command itself so we have a guaranteed null-terminated string
    int8_t filename[i + 1];
    strncpy((int8_t *) filename, (const int8_t *) command, i + 1);
    filename[i] = 0;

    int32_t entry_addr;
    // 3. Check executable format and load task image
    FILE f;
    if (fs_open(filename, &f) == -1) {
        return -1;
    }

    int8_t buf[BUF_SIZE];
    int32_t read_size = fs_file_read(buf, BUF_SIZE, &f);
    // Check for ELF magic
    if (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        fs_file_close(&f);
        return -1;
    }

    entry_addr = *((int32_t *) (buf + 24));
    // Check that the entry address is after the image starting point
    if (entry_addr < TASK_IMG_START_ADDR) {
        fs_file_close(&f);
        return -1;
    }

    // 4. Setup paging; Set task's target page address
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(task_count);
    // Reload the TLB
    asm volatile(
            "movl %%cr3, %%eax;"
            " movl %0, %%cr3; "
            :
            : "r"(page_directory)
            :"%eax"
            );

    // Copy the rest of the image
    uint8_t *task_img_cur = (uint8_t *) TASK_IMG_START_ADDR;
    memcpy(task_img_cur, buf, read_size);
    task_img_cur += read_size;
    while ((read_size = fs_file_read(buf, BUF_SIZE, &f)) > 0) {
        memcpy(task_img_cur, buf, read_size);
        task_img_cur += read_size;
    }

    fs_file_close(&f);

    // 5. Setup PCB
    PCB_t *task_pcb = (PCB_t *) TASK_KSTACK_TOP(task_count);
    for (i = 2; i < MAX_FILE_NUM; i ++) {
        task_pcb->open_files[i].flags.used = 0;
    }
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
    task_pcb->pid = task_count;

    tss.esp0 = TASK_KSTACK_BOT(task_count);
    tss.ss0 = KERNEL_DS;

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
        "movl %2, %%eax;" // User DS
        "push %%eax;"
        "movl %3, %%eax;" // Entry point
        "push %%eax;"
        "iret;"
"1:;"
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
        return term_read(buf, nbytes, NULL);
    }

    if (fd == 1) {
        return -1;
    }

    if (fd >= TASK_MAX_FILES) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    return task_pcb->open_files[fd].file_ops->read(
            buf, nbytes, &task_pcb->open_files[fd]);
}

int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {
    if (fd == 0) {
        return -1;
    }

    if (fd == 1) {
        return term_write(buf, nbytes, NULL);
    }

    if (fd >= TASK_MAX_FILES) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    return task_pcb->open_files[fd].file_ops->write(
            buf, nbytes, &task_pcb->open_files[fd]);
}

/* syscall_open
 *  Descrption: Add file descriptor on a process's PCB
 *
 *  Arg:
 *      filename: name of the target file
 *
 * 	RETURN:
 *      file descriptor number, which is a small non-negative number,
 *      0 if this file is an RTC file
 *      -1 if failed.
 */
int32_t syscall_open(const int8_t* filename) {
    dentry_t dent;
    if( read_dentry_by_name(filename, &dent) != 0 ){
        return -1;
    }

    // Need a curent pid to get the right PCB
    PCB_t *task_pcb = get_cur_pcb();
    int i;
    // determine the type of file
    switch( dent.filetype ){
        case FILE_TYPE_RTC:
            return rtc_open( &(task_pcb->rtc_info) );

        // open regular file
        case FILE_TYPE_REG:
        case FILE_TYPE_DIR:
            for (i = 2; i < TASK_MAX_FILES; i ++) {
                if (!(task_pcb->open_files[i].flags.used)) {
                    fs_open(filename, &task_pcb->open_files[i]);
                    return i;
                }
            }
            return -1;  // No usable fd
    }
    return -1;
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

PCB_t *get_cur_pcb() {
    uint32_t cur_esp;
    asm volatile ("movl %%esp, %0;" : "=r" (cur_esp));
    return (PCB_t *) (cur_esp & KSTACK_TOP_MASK);
}