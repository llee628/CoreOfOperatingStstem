#include "syscall.h"
#include "term.h"

int32_t syscall_isr(int32_t callnum, int32_t a, int32_t b, int32_t c) {
    switch (callnum) {
        case 1: return syscall_halt((uint8_t) (a & 0xFF));
        case 2: return syscall_execute((const uint8_t*) a);
        case 3: return syscall_read(a, (void *) b, c);
        case 4: return syscall_write(a, (void *) b, c);
        case 5: return syscall_open((const uint8_t*) a);
        case 6: return syscall_close(a);
        case 7: return syscall_getargs((uint8_t *) a, b);
        case 8: return syscall_vidmap((uint8_t **) a);
        case 9: return syscall_set_handler(a, (void *) b);
        case 10: return syscall_sigreturn();
        default: return -1;
    }
}

int32_t syscall_halt(uint8_t status) {
    return 0;
}

int32_t syscall_execute(const uint8_t* command) {
    return 0;
}

int32_t syscall_read(int32_t fd, void* buf, int32_t nbytes) {
    if (fd == 0) {
        return term_read(buf, nbytes);
    }

    if (fd == 1) {
        return -1;
    }
    return 0;
}

int32_t syscall_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (fd == 0) {
        return -1;
    }

    if (fd == 1) {
        return term_write(buf, nbytes);
    }
    return 0;
}

int32_t syscall_open(const uint8_t* filename) {
    return 0;
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
