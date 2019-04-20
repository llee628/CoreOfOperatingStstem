#include "scheduling.h"
#include "i8259.h"
#include "idt.h"

#DEFINE   8MB     0x800000
#DEFINE   8KB     0x2000

// Global variables
int active_term = 0;

void init_pit(){

    // Set interrupt handler
    SET_IDT_ENTRY(idt[PIT_INT], _pti_isr);
    idt[PIT_INT].present = 1;

    // Calculate 30 millisecond timer interrupt
    int32_t set_30MS = CLOCK_TICK_RATE / 33HZ_DIV;
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

    do{
    active_term = (active_term + 1) % 3;
    next_pid = cur_proc_term[active_term];
  } while( !next_pid )

    /* Return if there is no other process to schedule */
    if(cur_proc->pid == next_pid)
      return;

    PCB_t* next_proc = TASK_KSTACK_TOP(next_pid);

    /* Setup next process's paging */
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(next_pid);
    /* Flush TLB */
    asm volatile(
        " movl %0, %%cr3; "
        :
        : "r"(page_directory)
    );

    /* save current esp and ebp */
    asm volatile(
        " movl %%esp, %0; "
        " movl %%ebp, %0; "
        :
        :
    )

    /* Restore next process's esp and ebp */
    asm volatile(
        " movl %0, %%esp; "
        " movl %1, &&ebp; "
        :
        : "r"()
    )

    tss.esp0 = TASK_KSTACK_BOT(next_pid);
    tss.ss0 = KERNEL_DS;


}
