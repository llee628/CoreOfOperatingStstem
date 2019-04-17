#ifndef _SCHEDULING_H_
#define _SCHEDULING_H_

/* Relevant citations and sources
* http://www.osdever.net/bkerndev/Docs/pit.htm
* ULK ch.6 page 229-230
*/

#include "types.h"
#include "task.h"
#include "lib.h"
#include "syscall.h"

#DEFINE   CLOCK_TICK_RATE   1193182
#DEFINE   33HZ_DIV            36157

#DEFINE   PIT_DATA0_PORT     0x40
#DEFINE   PIT_CMD_REG        0x43

#DEFINE   PIT_MODE3          0x36

#DEFINE   PIT_IRQNUM         0



void init_pit(void);

void pit_isr(void);


#endif
