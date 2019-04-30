#include "serial.h"
#include "tuxctl.h"
#include "lib.h"

int rx_ind = 0;
int rx_buf[RX_PACKET_SIZE] = {0};

int tx_ind = 0;
int tx_dat_len = 0;
int tx_buf[TX_BUFFER_SIZE] = {0};

void init_serial() {
   outb(0x00, SERIAL_PORT + 1);    // Disable all interrupts
   outb(0x80, SERIAL_PORT + 3);    // Enable DLAB (set baud rate divisor)
   outb(0x0C, SERIAL_PORT + 0);    // Set divisor to 12 (lo byte) 9600 baud
   outb(0x00, SERIAL_PORT + 1);    //                   (hi byte)
   outb(0x03, SERIAL_PORT + 3);    // 8 bits, no parity, one stop bit
   outb(0xC7, SERIAL_PORT + 2);    // Enable FIFO, clear them, with 14-byte threshold
   outb(0x03, SERIAL_PORT + 4);    // IRQs enabled; Data Ready & Transmit empty
}

int data_ready() {
    return inb(SERIAL_PORT + 5) & 0x01;
}

int transmit_empty() {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_isr() {
    if (data_ready()) {
        rx_buf[rx_ind++] = inb(SERIAL_PORT);
        if (rx_ind == RX_PACKET_SIZE) {
            rx_ind = 0;
            tuxctl_handle_packet((unsigned char *) rx_buf);
        }
    }
    if (transmit_empty()) {
        if (tx_ind != tx_dat_len) {
            outb(tx_buf[tx_ind++], SERIAL_PORT);
        }
    }
}

void serial_write(unsigned char *buf, int size) {
    if (size >= TX_BUFFER_SIZE) {
        return;
    }
    memcpy(tx_buf, buf, size);
    tx_dat_len = size;
    tx_ind = 1;
    outb(tx_buf[0], SERIAL_PORT);
}
