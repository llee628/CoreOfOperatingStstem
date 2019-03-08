// frequency = 32768 >> (rate-1) = 1024 Hz, rate = 6
// 2 < rate < 15
#define RTC_RATE 6

void rtc_isr(void);
void init_rtc(void);
