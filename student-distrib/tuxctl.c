#include "tuxctl.h"
#include "mtcp.h"
#include "serial.h"
#include "term.h"
#include "kb.h"

int busy = 0;

// State of the button; written by BIOC events, read by ioctl
// format: right, down, left, up | c, b, a, start
// Buttons are active high
unsigned char button_state = 0xFF, old_button_state = 0xFF;

int tuxctl_ioctl(unsigned cmd, unsigned long arg);
void show_score(unsigned score);

void tuxctl_handle_packet(unsigned char *packet) {
    unsigned char a, b, c;

    a = packet[0];
    b = packet[1];
    c = packet[2];

    switch (a) {
        case MTCP_BIOC_EVENT:
            old_button_state = button_state;
            button_state = (((c & 0x09) | ((c & 0x04) >> 1) | ((c & 0x02) << 1)) << 4) | (b & 0x0F);
            switch ((button_state >> 4) & 0x0F) {
                case 0x01: term_key_handler((key_t) P('w')); break;
                case 0x02: term_key_handler((key_t) P('s')); break;
                case 0x08: term_key_handler((key_t) P('d')); break;
                case 0x04: term_key_handler((key_t) P('a')); break;
            }
            break;

        case MTCP_RESET:
            tuxctl_ioctl(TUX_INIT, 0);
            break;

        case MTCP_ACK:
            busy = 0;
            break;

        default:
            break;
    }
}

void init_tuxctl() {
    init_serial();
    tuxctl_ioctl(TUX_INIT, 0);
    tuxctl_ioctl(TUX_SET_SCORE, 0);
}

int tuxctl_ioctl(unsigned cmd, unsigned long arg) {
    static unsigned char buffer[256];
    unsigned char c;
    int i;
    switch (cmd) {
        case TUX_INIT:
            c = MTCP_BIOC_ON;
            while (busy);
            busy = 1;
            serial_write(&c, 1);
            return 0;

        case TUX_BUTTONS:
            if (!arg) {
                return -1;
            }
            // Inverse the buttons for easier processing
            *(unsigned char *) arg = ~button_state;
            return 0;

        case TUX_SET_SCORE:
            // Unwrap the time and update the LEDs
            show_score(arg);
            return 0;

        default:
            return -1;
    }
}

void show_score(unsigned score) {
    unsigned char buffer[8];
    if (score > 9999) {
        score = 9999;
    }
    unsigned digits;
    asm volatile (
        "mov %1, %%eax;"
        "da %%eax;"
        "mov %%eax, %0;"
        : "=g" (digits)
        : "g" (score)
        : "%eax"
    );
    buffer[0] = MTCP_LED_SET;
    buffer[1] = 0x0F;       // Always mask all of them
    buffer[2] = HEX_DATA[digits & 0x0F];
    buffer[3] = HEX_DATA[digits & (0x0F << 1)];
    buffer[4] = HEX_DATA[digits & (0x0F << 2)];
    buffer[5] = HEX_DATA[digits & (0x0F << 3)];
    while (busy);
    busy = 1;
    serial_write(buffer, 6);
}
