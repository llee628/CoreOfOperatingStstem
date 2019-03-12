#include "term.h"
#include "kb.h"
#include "lib.h"

uint8_t term_buf[TERM_BUF_SIZE];
uint8_t term_buf_count = 0;

void term_key_handler(uint8_t ch) {
    int i;
    if (ch < ' ') {     // Non-printable; probably a control character
        switch (ch) {
            case '\x0C':    // ^L; clear
                term_buf_count = 0;
                for (i = getposy(); i > 0; i --) {
                    scroll();
                }
                clear();
                break;

            case '\x17':    // ^W; kill word
                if (!term_buf_count) {
                    break;
                }

                if (isalnum(term_buf[term_buf_count - 1])) {
                    while (term_buf_count && isalnum(term_buf[term_buf_count - 1])) {
                        delch();
                    }
                } else {
                    while (term_buf_count && !isalnum(term_buf[term_buf_count - 1])) {
                        delch();
                    }
                }
                break;

            case '\x15':    // ^U; kill line
                while (term_buf_count > 0) {
                    delch();
                }
                break;

            case '\x08':    // ^H; kill char
                delch();
                break;

            case '\x03':    // ^C; keyboard interrupt
                puts("^C\n");
                term_buf_count = 0;
                puts(" => ");
                break;

            case '\n':      // ^M / ^J; Return
                // TODO next line
                putc('\n');
                puts("Line buf Content: ");
                for (i = 0; i < term_buf_count; i ++) {
                    putc(term_buf[i]);
                }
                term_buf_count = 0;
                putc('\n');
                puts(" => ");
                break;

            case '\t':
                // TODO tab
                break;
        }
    } else {
        addch(ch);
    }
}

// Add a character to the line buf
void addch(uint8_t ch) {
    if (term_buf_count < TERM_BUF_SIZE) {
        term_buf[term_buf_count++] = ch;
        putc(ch);
    }
}

// delete a character to the line buf
void delch() {
    if (term_buf_count > 0) {
        term_buf_count --;
        putc('\b');
    }
}

void init_term() {
    cli();
    outb(0x0A, 0x3D4);
    outb(0x00, 0x3D5);  // Enable cursor; cursor scanline start at 0
    outb(0x0B, 0x3D4);
    outb(0x0F, 0x3D5);  // No cursor skew; cursor scanline ends at 15 => blocky cursors
    sti();
}
