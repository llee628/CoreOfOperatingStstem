#include "scheduling.h"
#include "i8259.h"


void init_pit(){
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

}
