#ifndef _RTC_H_
#define _RTC_H_


#ifndef ASM


#include "types.h"
#include "task.h"
#include "rtc_info.h"

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

#define RTC_SYS_MX_FREQ 8192
#define RTC_USR_MX_FREQ 1024
#define RTC_USR_DEFAULT_FREQ 2

file_ops_table_t rtc_file_ops_table;

void rtc_isr(void);
void init_rtc(void);
int32_t rtc_set_pi_freq(int32_t freq); //set RTC Hardware freq

//int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_open(rtc_info_t *rtc);
int32_t rtc_read(rtc_info_t *rtc);
int32_t rtc_write_usr(rtc_info_t *rtc, int32_t freq);
int32_t rtc_close(rtc_info_t *rtc);

#endif

#endif
