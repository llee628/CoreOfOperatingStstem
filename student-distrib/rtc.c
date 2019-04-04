// vim:ts=4 noet
#include "rtc.h"
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"

#define RTC_SYS_START_FREQ 2

int int_flag = 0;	//flag used to check if interrupt handler is called appropriately
static int32_t rtc_max_user_freq = RTC_USR_MX_FREQ;
static rtc_info_t *cur_rtc ;

// internal structure
static int32_t cur_sys_int_freq;
static int32_t int_cnt;
static int32_t int_target_count;

static int8_t is_valid_freq( int32_t freq, int8_t is_sys);
static int32_t get_target_count( int32_t sys_int_freq, int32_t target_int_freq);

static int32_t get_target_count( int32_t sys_int_freq, int32_t target_int_freq){
	int i;
	for( i=0; sys_int_freq > (target_int_freq << i); i++){
		;
	}
	return 1<< i;
}


void init_rtc(void) {
	char prev;
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
	prev = inb(RTC_DATA_PORT);		// read the current value of register B
	outb(RTC_REG_B, RTC_ADDR_PORT);		// set the index again (a read will reset the index to register D)
	outb(prev | 0x40, RTC_DATA_PORT);// write the previous value ORed with 0x40. This turns on bit 6 of register B

	rtc_set_pi_freq(RTC_SYS_START_FREQ);
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
	if( is_valid_freq(freq,1) != 1){
		return -1;
	}

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
	cur_sys_int_freq = rtc_rate_val; 
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
int32_t rtc_write_usr(rtc_info_t *rtc, int32_t freq){
	if(rtc==NULL || is_valid_freq(freq, 0) != 1){
		return -1;
	}

	rtc->waiting = 0;

	//update sys freq 
	if( freq > cur_sys_int_freq ){
		rtc_set_pi_freq( freq);
	}
	rtc->sys_int_freq = cur_sys_int_freq;
	rtc->target_int = get_target_count( rtc->sys_int_freq, freq);
	rtc->freq = freq;
	return 0;
}

void rtc_isr(void) {
	outb(0x0C, RTC_ADDR_PORT);
	(void) inb(RTC_DATA_PORT);
	if( cur_rtc != NULL && cur_rtc->waiting ==1 )
	{
		int_cnt++;
		if( int_cnt > int_target_count ){
			cur_rtc->waiting =0;
			//test_rtc_freq(0);
		}
	}

}


int32_t rtc_read(rtc_info_t *rtc){
	if( rtc == NULL || !(rtc->valid)  ){return -1;}
	
	if( rtc->sys_int_freq != cur_sys_int_freq ){
		rtc->sys_int_freq = cur_sys_int_freq;
		rtc->target_int = get_target_count( rtc->sys_int_freq, rtc->freq);
	}

	int_target_count = rtc->target_int;
	int_cnt = 0;
	rtc->waiting = 1;
	cur_rtc = rtc;
	while( cur_rtc->waiting ){
		;
	} 
	cur_rtc = NULL;
    return 0;
}

int32_t rtc_open( rtc_info_t *rtc ){
	if( rtc!= NULL && rtc->valid == 0 ){
		rtc->valid = 1;
		rtc->waiting =0;
		rtc->freq = RTC_USR_DEFAULT_FREQ;
		rtc->sys_int_freq =  cur_sys_int_freq;

		rtc_write_usr(rtc, rtc->freq);
		return 0;
	}else{
		return -1;
	}
}

int32_t rtc_close(rtc_info_t *rtc){
	if( rtc!= NULL ){
		if( cur_rtc == rtc ){
			cur_rtc = NULL;
		}

		// The system might have use slower RTC freq instead
		if( rtc->freq == cur_sys_int_freq ){
			rtc_set_pi_freq( RTC_SYS_START_FREQ);
		}
		rtc->valid =0;
		return 0;
	}else{
		return -1;
	}
    return 0;
}


static int8_t is_valid_freq( int32_t freq, int8_t is_sys){
	if ( (freq<2) || (freq>RTC_SYS_MX_FREQ) ){
		return 0;
	}
	// limit user frequency
	if ( !is_sys && freq > rtc_max_user_freq) {
		return 0;
	}

	//Check that the input was really a power of 2.
	int tmp =1;
	while (freq > (1<<tmp))
		tmp++;
	if (freq != (1<<tmp))
		return 0;
	return 1;
}
