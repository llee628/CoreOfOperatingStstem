#ifndef TUXCTL_H
#define TUXCTL_H

typedef enum {
    TUX_SET_LED,
    TUX_READ_LED,
    TUX_BUTTONS,
    TUX_INIT,
    TUX_LED_REQUEST,
    TUX_LED_ACK,
    TUX_SET_SCORE,
} tux_cmd_t;

void tuxctl_handle_packet(unsigned char *packet);
void init_tuxctl();
void show_score(unsigned score);

unsigned char HEX_DATA[16] = {
    0xE7, // 0
    0x06, // 1
    0xCB, // 2
    0x8F, // 3
    0x2E, // 4
    0xAD, // 5
    0xED, // 6
    0x86, // 7
    0xEF, // 8
    0xAF, // 9
    0xE6, // A
    0x6D, // b
    0xE1, // C
    0x4F, // d
    0xE9, // E
    0xE8, // F
};

#endif
