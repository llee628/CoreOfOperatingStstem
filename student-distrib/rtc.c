// vim:ts=4 noet
#include "rtc.h"
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"

void init_rtc(void) {
	char prev;
	cli();
	// Init routine from https://wiki.osdev.org/RTC
	// Enable interrupt
	outb(0x8B, 0x70);		// select register B, and disable NMI
	prev = inb(0x71);		// read the current value of register B
	outb(0x8B, 0x70);		// set the index again (a read will reset the index to register D)
	outb(prev | 0x40, 0x71);// write the previous value ORed with 0x40. This turns on bit 6 of register B

	// Set frequency to 1024 Hz
	outb(0x8A, 0x70);		// set index to register A, disable NMI
	prev = inb(0x71);		// get initial value of register A
	outb(0x8A, 0x70);		// reset index to A
	outb((prev & 0xF0) | RTC_RATE, 0x71); //write only our rate to A. Note, rate is the bottom 4 bits.

	// Set interrupt handler
	SET_IDT_ENTRY(idt[0x28], rtc_isr);
	idt[0x28].present = 1;

	// Enable IRQs
	enable_irq(RTC_IRQ);
	sti();
}

void rtc_isr(void) {
	test_interrupts();
}
