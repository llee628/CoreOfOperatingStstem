#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "lib.h"

int DR, TE;

#define SERIAL_PORT 0x3f8
#define RX_PACKET_SIZE 3
#define TX_BUFFER_SIZE 16

void init_serial();
int data_ready();
int transmit_empty();
void serial_isr();
void serial_write(unsigned char *buf, int size);

#endif /* ifndef _SERIAL_H_ */
