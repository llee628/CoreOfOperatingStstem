#ifndef _RTC_H_
#define _RTC_H_


#ifndef ASM

#include "types.h"
// Set rtc periodic interruput freq
extern int rtc_set_pi_freq(int32_t freq);

// frequency = 32768 >> (rate-1) = 1024 Hz, rate = 6
// 2 < rate < 15
#define RTC_RATE 6
#define RTC_OPEN_RATE   15      //set rtc frequency to 2Hz

// I/O port 
#define RTC_ADDR_PORT 0x70 
#define RTC_DATA_PORT 0x71

// index of registe number in RTC
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C

#define RTC_FREQ_SELECT RTC_REG_A



void rtc_isr(void);
void init_rtc(void);
//int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_read();
int32_t rtc_open();
int32_t rtc_close();


#endif

#endif
