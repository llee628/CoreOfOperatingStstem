
#include "rtc_read.h"
#include "rtc.h"
#include "lib.h"

#define RTC     1
extern int int_flag;

int32_t read (int32_t fd, void* buf, int32_t nbytes){
    
    while (1){
        if (int_flag == 1)
            return 0;
    }
   
    
    
    
}
