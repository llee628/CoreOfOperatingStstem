
#include "rtc_read.h"
#include "rtc.h"
#include "lib.h"

#define RTC     1

int32_t read (int32_t fd, void* buf, int32_t nbytes){
    
    if (fd == RTC)
    {
        if (int_flag == 0)
            return 0;
    }
    
}
