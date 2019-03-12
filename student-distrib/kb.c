#include "lib.h"
#include "idt.h"
#include "i8259.h"
#include "x86_desc.h"
#include "term.h"
#include "kb.h"

static uint8_t shift, ctrl, caps;

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
          0,    27,   '1',   '2',   '3',   '4',   '5',   '6',
        '7',   '8',   '9',   '0','\x1F',   '=',  '\b',  '\t',
     '\x11','\x17','\x05','\x12','\x14','\x19','\x15',  '\t',
     '\x0F','\x10','\x1B','\x1D',  '\n',     0, /* Control */
     '\x01','\x13','\x04','\x06','\x07',  '\b',  '\n','\x0B',
     '\x0C',   ';',  '\'','\x0A',     0, /* Left shift */
     '\x1C','\x1A','\x18','\x03','\x16','\x02','\x0E',  '\n',
        ',',   '.',  '',     0, /* Right shift */
        '*',     0, /* Alt */
     '\x0A',
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
    // Key is pressed if MSB is set
    uint8_t pressed = !(keycode & 0x80);
    keycode = keycode & 0x7F;
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

            term_key_handler(keyboard_map[keycode]);
    }
}
