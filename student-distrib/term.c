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
    term_curpos = 0;
    while (!term_read_done) {
        asm volatile ("hlt");
    }
    term_key_handler(0x05);
    putc('\n');
    term_buf[term_buf_count++] = '\n';
    return term_buf_count;
}

int32_t term_write(const void* buf, int32_t nbytes) {
    int i;
    static uint8_t state = 0;
    for (i = 0; i < nbytes; i ++) {
        uint8_t c = ((char * ) buf)[i];
        if (c == 0x1b) {
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
            case 0x0C:      // C-L; clear
                term_buf_count = 0;
                for (i = getposy(); i > 0; i --) {
                    scroll();
                }
                break;

            case 0x02:      // C-B; char back
                if (term_curpos > 0) {
                    term_curpos --;
                    back();
                }
                break;

            case 0x06:      // C-F; char forward
                if (term_curpos < term_buf_count) {
                    term_curpos ++;
                    forward();
                }
                break;

            case 0x01:      // C-A; start of line
                while (term_curpos > 0) {
                    term_curpos --;
                    back();
                }
                break;

            case 0x05:      // C-E; end of line
                while (term_curpos < term_buf_count) {
                    term_curpos ++;
                    forward();
                }
                break;

            case 0x17:      // C-W; kill word
                if (!term_curpos) {
                    break;
                }

                if (isalnum(term_buf[term_curpos - 1])) {
                    while (term_curpos && isalnum(term_buf[term_curpos - 1])) {
                        delch();
                    }
                } else {
                    while (term_curpos && !isalnum(term_buf[term_curpos - 1])) {
                        delch();
                    }
                }
                break;

            case 0x15:      // C-U; kill line
                while (term_curpos > 0) {
                    delch();
                }
                break;

            case 0x08:      // C-H; kill char
                delch();
                break;

            case 0x03:      // C-C; keyboard interrupt
                puts("^C");
                term_read_done = 1;
                term_buf_count = 0;
                break;

            case '\n':      // C-M / C-J; Return
                term_read_done = 1;
                break;

            case '\t':
                // TODO tab
                break;
        }
    } else if (ch & 0x80) { // An alt'd character
        switch (ch) {
            case 0x82:      // M-B; word back
                if (!term_curpos) {
                    break;
                }

                if (isalnum(term_buf[term_curpos - 1])) {
                    while (term_curpos && isalnum(term_buf[term_curpos - 1])) {
                        term_curpos --;
                        back();
                    }
                } else {
                    while (term_curpos && !isalnum(term_buf[term_curpos - 1])) {
                        term_curpos --;
                        back();
                    }
                }
                break;

            case 0x86:      // M-F; word forward
                if (term_curpos >= term_buf_count) {
                    break;
                }

                if (isalnum(term_buf[term_curpos])) {
                    while ((term_curpos < term_buf_count)
                            && isalnum(term_buf[term_curpos])) {
                        term_curpos ++;
                        forward();
                    }
                } else {
                    while ((term_curpos < term_buf_count)
                            && !isalnum(term_buf[term_curpos])) {
                        term_curpos ++;
                        forward();
                    }
                }
                break;
        }
    } else {
        addch(ch);
    }
}

// Add a character to the line buf
void addch(uint8_t ch) {
    if (term_buf_count < term_buf_size) {
        int i;
        for (i = term_buf_count; i > term_curpos; i --) {
            term_buf[i] = term_buf[i - 1];
        }
        term_buf[term_curpos++] = ch;
        term_buf_count ++;
        putc(ch);
        for (i = term_curpos; i < term_buf_count; i ++) {
            putc(term_buf[i]);
        }
        for (i = term_buf_count - term_curpos; i > 0; i --) {
            back();
        }
    }
}

// delete a character to the line buf
void delch() {
    if (term_curpos > 0) {
        int i;
        for (i = term_curpos - 1; i < term_buf_count - 1; i ++) {
            term_buf[i] = term_buf[i + 1];
        }
        term_curpos --;
        term_buf_count --;
        back();
        int old_x = getposx(), old_y = getposy();
        for (i = term_curpos; i < term_buf_count; i ++) {
            putc(term_buf[i]);
        }
        putc(' ');
        setpos(old_x, old_y);
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
