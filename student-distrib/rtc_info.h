#ifndef _RTC_INFO_H_
#define _RTC_INFO_H_
#ifndef ASM

typedef struct rtc_info_s{
	int valid;
	int32_t freq;

	// internal data

	int waiting;
	int32_t sys_int_freq;
	int32_t target_int;
}rtc_info_t;

#endif

#endif
