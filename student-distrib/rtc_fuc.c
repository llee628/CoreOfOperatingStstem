
#include "rtc_fuc.h"
#include "rtc.h"
#include "lib.h"

#define RTC     1
extern int int_flag;
//int32_t read (int32_t fd, void* buf, int32_t nbytes)


int32_t rtc_read (){
    while (1){
        if (int_flag == 0)
            return 0;
    }

}
