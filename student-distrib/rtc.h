#ifndef _RTC_H_
#define _RTC_H_
#include "types.h"
// frequency = 32768 >> (rate-1) = 1024 Hz, rate = 6
// 2 < rate < 15
#define RTC_RATE 6
#define RTC_OPEN_RATE   14      //set rtc frequency to 2Hz

#ifndef ASM

void rtc_isr(void);
void init_rtc(void);
//int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_read();
int32_t rtc_open();
int32_t rtc_close();


#endif

#endif
