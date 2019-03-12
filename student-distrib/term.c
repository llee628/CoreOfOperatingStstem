#include "term.h"
#include "kb.h"
#include "lib.h"

uint8_t term_buf_size = 0;
char *term_buf = NULL;
uint8_t term_buf_count = 0;
uint8_t term_read_done = 0;
uint8_t term_curpos = 0;

void addch(uint8_t ch);
void delch();

int32_t term_read(void* buf, int32_t nbytes) {
    term_buf = (char *) buf;
    term_buf_size = nbytes - 1;     // Reserve space for newline
    term_buf_count = 0;
    term_read_done = 0;
    while (!term_read_done);
    term_buf[term_buf_count++] = '\n';
    return term_buf_count;
}

int32_t term_write(const void* buf, int32_t nbytes) {
    int i;
    static uint8_t state = 0;
    for (i = 0; i < nbytes; i ++) {
        uint8_t c = ((char * ) buf)[i];
        if (c == '\x1b') {
            state = 1;
            continue;
        }
        if (c == '[') {
            state = 2;
            continue;
        }
        if (state == 2 || state == 3) {
            if (c == 'x') {
                if (state == 2) {
                    setattr((getattr() & 0xF0) | (DEF_ATTR & 0x0F));
                    state ++;
                } else if (state == 3) {
                    setattr((getattr() & 0x0F) | (DEF_ATTR & 0xF0));
                    state = 0;
                }
                continue;
            }
            if (c >= '0' && c <= '9') {
                c -= '0';
            } else if (c >= 'A' && c <= 'F') {
                c -= 'A';
                c += 10;
            } else if (c >= 'a' && c <= 'f') {
                c -= 'a';
                c += 10;
            }
            if (state == 2) {
                setattr((getattr() & 0xF0) | (c & 0x0F));
                state ++;
            } else if (state == 3) {
                setattr((getattr() & 0x0F) | (c & 0xF0));
                state = 0;
            }
            continue;
        }
        state = 0;

        putc(c);
    }
    return nbytes;
}

void term_key_handler(uint8_t ch) {
    int i;
    if (ch < ' ') {     // Non-printable; probably a control character
        switch (ch) {
            case '\x0C':    // ^L; clear
                term_buf_count = 0;
                for (i = getposy(); i > 0; i --) {
                    scroll();
                }
                break;

            case '\x02':    // ^B; char back
                break;

            case '\x06':    // ^F; char forward
                break;

            case '\x01':    // ^A; start of line
                break;

            case '\x05':    // ^E; end of line
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
                term_read_done = 1;
                term_buf_count = 0;
                //puts(" => ");
                break;

            case '\n':      // ^M / ^J; Return
                putc('\n');
                term_read_done = 1;
                // Test code
                //puts("Line buf Content: ");
                //for (i = 0; i < term_buf_count; i ++) {
                //    putc(term_buf[i]);
                //}
                //term_buf_count = 0;
                //putc('\n');
                //puts(" => ");
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
    if (term_buf_count < term_buf_size) {
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
