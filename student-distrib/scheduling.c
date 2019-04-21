#include "scheduling.h"
#include "i8259.h"
#include "idt.h"
#include "x86_desc.h"
#include "term.h"

#define   _8MB     0x800000
#define   _8KB     0x2000

// Global variables

void init_pit(){

    // Set interrupt handler
    SET_IDT_ENTRY(idt[PIT_INT], _pti_isr);
    idt[PIT_INT].present = 1;

    // Calculate 30 millisecond timer interrupt
    int32_t set_30MS = CLOCK_TICK_RATE / _33HZ_DIV;
    // Set pit mode to a square wave
    outb(PIT_MODE3, PIT_CMD_REG);
    // Set low bits
    outb(set_30MS & 0xFF, PIT_DATA0_PORT);
    // Set high bits
    outb(set_30MS >> 8, PIT_DATA0_PORT);

    enable_irq(PIT_IRQNUM);
}

void pit_isr(){
    /* Send an eoi first as always */
    send_eoi(PIT_IRQNUM);

    uint8_t next_pid;
    PCB_t* cur_proc = get_cur_pcb();

    // Sanity check
    if(!cur_proc)
        return;

    /* do{ */
    /*     active_term = (active_term + 1) % 3; */
    /*     next_pid = terms[active_term].cur_pid; */
    /* } while( !next_pid ); */
    cur_term_ind = (cur_term_ind + 1) % 3;
    if (!terms[cur_term_ind].cur_pid) {
        _syscall_execute("shell", cur_term_ind);
    }
    else{
      next_pid = terms[cur_term_ind].cur_pid;
    }

    /* Return if there is no other process to schedule */
    if(cur_proc->pid == next_pid)
        return;

    PCB_t* next_proc = (PCB_t *) TASK_KSTACK_TOP(next_pid);

    /* Setup next process's paging */
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(next_pid);
    tss.esp0 = TASK_KSTACK_BOT(next_pid);
    cur_term_ind = next_proc->term_ind;
    cur_term = &terms[cur_term_ind];
    /* Flush TLB */
    asm volatile(
        " movl %0, %%cr3; "
        :
        : "r"(page_directory)
    );

    /* save current esp and ebp */
    asm volatile(
        " movl %%esp, %0; "
        " movl %%ebp, %1; "
        : "=m" (cur_proc->esp), "=m" (cur_proc->ebp)
    );

    /* Restore next process's esp and ebp */
    asm volatile(
        " movl %0, %%esp; "
        " movl %1, %%ebp; "
        :
        : "m" (next_proc->esp), "m" (next_proc->ebp)
    );
}
