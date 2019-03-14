#include "lib.h"
#include "idt.h"
#include "i8259.h"
#include "x86_desc.h"
#include "term.h"
#include "kb.h"

static uint8_t shift, ctrl, alt, caps, scan_state = 0;

// Dirty keyboard interpretation code
// Borrowed from https://stackoverflow.com/a/37635449/5264490
uint8_t keyboard_map[PRINT_KEY_NUM] = {
       0,  27, '1','2', '3','4', '5', '6',
     '7', '8', '9','0', '-','=','\b','\t',
     'q', 'w', 'e','r', 't','y', 'u', 'i',
     'o', 'p', '[',']','\n',  0, /* Control */
     'a', 's', 'd','f', 'g','h', 'j', 'k',
     'l', ';','\'','`',   0, /* Left shift */
    '\\', 'z', 'x','c', 'v','b', 'n', 'm',
     ',', '.', '/',   0, /* Right shift */
     '*',   0, /* Alt */
     ' ',
};

uint8_t shift_map[PRINT_KEY_NUM] = {
       0,  27, '!','@', '#','$', '%', '^',
     '&', '*', '(',')', '_','+','\b','\t',
     'Q', 'W', 'E','R', 'T','Y', 'U', 'I',
     'O', 'P', '{','}','\n',  0, /* Control */
     'A', 'S', 'D','F', 'G','H', 'J', 'K',
     'L', ':', '"','~',   0, /* Left shift */
     '|', 'Z', 'X','C', 'V','B', 'N', 'M',
     '<', '>', '?',  0, /* Right shift */
     '*',   0, /* Alt */
     ' ',
};

uint8_t ctrl_map[PRINT_KEY_NUM] = {
       0,   27,  '1',  '2',  '3',  '4',  '5',  '6',
     '7',  '8',  '9',  '0', 0x1F,  '=', '\b', '\t',
    0x11, 0x17, 0x05, 0x12, 0x14, 0x19, 0x15, '\t',
    0x0F, 0x10, 0x1B, 0x1D, '\n',    0,  /* Control */
    0x01, 0x13, 0x04, 0x06, 0x07, '\b', '\n', 0x0B,
    0x0C,  ';', '\'', 0x0A,    0,  /* Left shift */
    0x1C, 0x1A, 0x18, 0x03, 0x16, 0x02, 0x0E, '\n',
     ',',  '.', '',    0,  /* Right shift */
     '*',    0,  /* Alt */
    0x0A, 
};

void init_kb(void) {
    cli();

	enable_irq(KB_IRQ);
	// Set interrupt handler
	SET_IDT_ENTRY(idt[KB_INT], _kb_isr);
	idt[KB_INT].present = 1;

    sti();
}

void kb_isr(void) {
    uint8_t status;
    uint8_t keycode;

    /* Acknowledgment */
    status = inb(0x64);
    /* Lowest bit of status will be set if buffer is not empty */
    if (!(status & 0x01)) {
        return;
    }
    keycode = inb(0x60);
    if (keycode == 0xE0) {
        scan_state = 1;
        return;
    }
    // Key is pressed if MSB is set
    uint8_t pressed = !(keycode & 0x80);
    keycode = keycode & 0x7F;
    if (scan_state == 0) {
        /* Only print characters on keydown event that have
         * a non-zero mapping */
        switch (keycode) {
            case 0x2A:      // Left shift
            case 0x36:      // Right shift
                shift = pressed;
                break;

            case 0x1D:      // Left control
                ctrl = pressed;
                break;

            case 0x38:      // Left alt
                alt = pressed;
                break;

            case 0x3A:      // Capslock
                caps ^= 1;
                break;
            default:
                if (keycode >= PRINT_KEY_NUM) { // We've got a key we can't handle
                    return;
                }

                if (!pressed) {
                    return;
                }
                if ((caps ^ shift) 
                        && keyboard_map[keycode] >= 'A' 
                        && keyboard_map[keycode] <= 'Z') {  // Emit uppercase letters when only CAPS or shift is on
                    term_key_handler(shift_map[keycode]);
                    break;
                }
                if (shift) {
                    term_key_handler(shift_map[keycode]);
                    break;
                }

                if (ctrl) {
                    term_key_handler(ctrl_map[keycode]);
                    break;
                }

                if (alt) {
                    term_key_handler(ctrl_map[keycode] + 0x80);
                    break;
                }

                term_key_handler(keyboard_map[keycode]);
        }
    } else if (scan_state == 1) {
        switch (keycode) {
            case 0x1D:      // Right control
                ctrl = pressed;
                break;

            case 0x38:      // Right alt
                alt = pressed;
                break;

            case 0x4B:      // Left arrow
                if (pressed) {
                    term_key_handler(0x02);
                }
                break;

            case 0x4D:      // Right arrow;
                if (pressed) {
                    term_key_handler(0x06);
                }
                break;
        }
        scan_state = 0;
    }
}
