#include "signals.h"
#include "syscall.h"
#include "x86_desc.h"
#include "lib.h"

sighandler_t *signal_handlers[SIG_SIZE];
sighandler_t *def_signal_handlers[SIG_SIZE];
sighandler_t sighandler_kill_task;

void init_signals() {
    cli();
    def_signal_handlers[0] = sighandler_kill_task;
    def_signal_handlers[1] = sighandler_kill_task;
    def_signal_handlers[2] = sighandler_kill_task;
    def_signal_handlers[3] = sighandler_ignore;
    def_signal_handlers[4] = sighandler_ignore;

    uint8_t i;
    for (i = 0; i < SIG_SIZE; i ++) {
        signal_handlers[i] = def_signal_handlers[i];
    }
    sti();
}

void sighandler_ignore() {
}

void sighandler_kill_task() {
    do_syscall(1, 0, 0, 0);
}

void check_signals(hw_context_t *context) {
    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->signals) {
        return;
    }

    // Only deliver signal when return to user
    if (context->cs != USER_CS) {
        return;
    }

    uint8_t i;
    for (i = 0; i < SIG_SIZE; i ++) {
        uint8_t mask = SIG_FLAG(i);
        if (task_pcb->signals & mask) {
            task_pcb->signals &= ~mask;
            uint32_t *user_esp = context->esp;
            user_esp -= 17;
            hw_context_t *saved_context = (hw_context_t *) user_esp;
            *saved_context = *context;

            user_esp -= 1;
            *user_esp = (uint32_t) i;
            user_esp -= 1;
            *user_esp = (uint32_t) &sigreturn_linkage;
            context->esp = user_esp;
            context->addr = signal_handlers[i];
            return;
        }
    }
}
