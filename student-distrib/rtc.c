// vim:ts=4 noet
#include "rtc.h"
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "task.h"
#include "syscall.h"

#define RTC_SYS_START_FREQ 2

static FILE *rtc_files[MAX_PROC_NUM] = {NULL};

file_ops_table_t rtc_file_ops_table = {
    .open = rtc_open,
    .read = rtc_read,
    .write = rtc_write,
    .close = rtc_close,
};

void init_rtc(void) {
	cli();
	// Set interrupt handler
	SET_IDT_ENTRY(idt[RTC_INT], _rtc_isr);
	idt[RTC_INT].present = 1;

	// Enable IRQs
	enable_irq(RTC_IRQ);
	enable_irq(SLAVE_PIC_IRQ);

	// Init routine from https://wiki.osdev.org/RTC
	// Enable interrupt
	outb(RTC_REG_B, RTC_ADDR_PORT);		// select register B, and disable NMI
	char prev = inb(RTC_DATA_PORT);		// read the current value of register B
	outb(RTC_REG_B, RTC_ADDR_PORT);		// set the index again (a read will reset the index to register D)
	outb(prev | 0x40, RTC_DATA_PORT);	// write the previous value ORed with 0x40. This turns on bit 6 of register B

	rtc_set_pi_freq(RTC_SYS_MAX_FREQ);
	sti();
}

/* rtc_set_pi_freq
 *	Descrption:	Set the RTC Hardware periodic interrupt frequency
 * 	
 *	Arg: freq: must be a power of 2 and inside the range of [2,8192]
 *		isSys: if the caller is system (system can reach 8192Hz, however user can
 * 			only reach 1024hz)
 * 	RETURN:
 * 		-1 if failed
 * 		0  if sucess
 *	reference :https://github.com/torvalds/linux/blob/master/drivers/char/rtc.c
 */
int32_t rtc_set_pi_freq(int32_t freq){
	int tmp, rtc_rate_val;
	char prev;

	//Calculate real rtc register value
	tmp =1;
	while (freq > (1<<tmp))
		tmp++;
	if (freq != (1<<tmp))
		return -1;
	// set rtc
	rtc_rate_val = 16 - tmp; // 8192 is the 2 to the power of 16.

	// set frequency	
	cli();
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// set index to register A, disable NMI
	prev = inb(RTC_DATA_PORT);				// get initial value of register A
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// reset index to A
	outb((prev & 0xF0) | (rtc_rate_val&0xF), RTC_DATA_PORT);	//write only our rate to A. Note, rate is the bottom 4 bits.
	sti();
	/* cur_sys_int_freq = rtc_rate_val; */ 
	return 0;
}

/* rtc_write_usr
 *	Descrption:	Set the frequency of user RTC
 * 	
 *	Arg: freq: must be a power of 2 and inside the range of [2,8192]
 *		isSys: if the caller is system (system can reach 8192Hz, however user can
 * 			only reach 1024hz)
 * 	RETURN:
 * 		-1 if failed
 * 		0  if sucess
 */
int32_t rtc_write(const int8_t *buf, uint32_t length, FILE *file){
	if (length < 4 || !buf) {
		return -1;
	}

	uint32_t freq = *(uint32_t *) buf;
	if (freq > RTC_USER_MAX_FREQ) {
		return -1;
	}

	file->inode = RTC_SYS_MAX_FREQ / freq;

	return 0;
}

void rtc_isr(void) {
	outb(0x0C, RTC_ADDR_PORT);
	(void) inb(RTC_DATA_PORT);
	uint8_t i;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		FILE *cur_file = rtc_files[i];
		if (cur_file) {
			if (cur_file->pos == cur_file->inode) {
				cur_file->pos = 0;
			} else {
				cur_file->pos ++;
			}
		}
	}
}


int32_t rtc_read(int8_t* buf, uint32_t length, FILE *file){
	sti();
	while (file->pos != file->inode) {
		asm volatile ("hlt");
	}
	cli();
	return 0;
}

int32_t rtc_open(const int8_t *filename, FILE *file){
	// `pos' as the counter
    file->pos = 0;
	// `inode' as the counter to match against
    file->inode = RTC_USER_DEF_FREQ;
	file->file_ops = &rtc_file_ops_table;
	file->flags.type = TASK_FILE_RTC;

	uint8_t i;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		if (!rtc_files[i]) {
			rtc_files[i] = file;
			return 0;
		}
	}

	while (1) { asm volatile ("hlt;"); } 	// UNREACHABLE!!!
}

int32_t rtc_close(FILE *file){
	uint8_t i;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		if (rtc_files[i] == file) {
			rtc_files[i] = NULL;
		}
	}
	return 0;
}
