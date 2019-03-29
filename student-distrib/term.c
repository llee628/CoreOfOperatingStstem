#include "term.h"
#include "kb.h"
#include "lib.h"

// Buffer need to include the newline, so this one reserves space for the newline
#define TERM_BUF_SIZE 127
#define TERM_BUF_SIZE_W_NL 128
char term_buf[TERM_BUF_SIZE_W_NL];
uint8_t term_buf_count = 0;
uint8_t term_read_done = 0;
uint8_t term_curpos = 0;

// File ops table
file_ops_table_t term_file_ops_table = {
    .open = term_open,
    .read = term_read,
    .write = term_write,
    .close = term_close,
};

void addch(uint8_t ch);
void delch();

// Dummy open and close functions
int32_t term_open(const uint8_t *filename) {
    return 0;
}

int32_t term_close() {
    return 0;
}

int32_t term_read(void* buf, int32_t nbytes) {
    if (!buf) {
        return -1;
    }
    sti();
    while (!term_read_done) {
        asm volatile ("hlt");
    }
    cli();
    putc('\n');
    term_buf[term_buf_count++] = '\n';
    int copy_count = nbytes < term_buf_count ? nbytes : term_buf_count;
    memcpy(buf, term_buf, copy_count);
    term_buf_count = 0;
    term_read_done = 0;
    term_curpos = 0;
    return copy_count;
}

int32_t term_write(const void* buf, int32_t nbytes) {
    int i;
    static uint8_t state = 0;
    for (i = 0; i < nbytes; i ++) {
        uint8_t c = ((char * ) buf)[i];
        // Simple state machine to parse escape codes with a format of ^[[...
        // only colors for now, with a format of ^[[\x\x or ^[[xx
        // first digit is foreground color, second is background
        // if 'x' is used, then default color is used.
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

void term_key_handler(key_t key) {
    int i;
    if (key.modifiers == MOD_CTRL) {     // Non-printable; probably a control character
        switch (key.key) {
            case 'l':      // C-L; clear
                for (i = 0; i < term_curpos; i ++) {
                    back();
                }
                for (i = getposy(); i > 0; i --) {
                    scroll();
                }
                for (i = 0; i < term_curpos; i ++) {
                    forward();
                }
                break;

            case 'b':      // C-B; char back
                if (term_curpos > 0) {
                    term_curpos --;
                    back();
                }
                break;

            case 'f':      // C-F; char forward
                if (term_curpos < term_buf_count) {
                    term_curpos ++;
                    forward();
                }
                break;

            case 'a':      // C-A; start of line
                while (term_curpos > 0) {
                    term_curpos --;
                    back();
                }
                break;

            case 'e':      // C-E; end of line
                while (term_curpos < term_buf_count) {
                    term_curpos ++;
                    forward();
                }
                break;

            case 'w':      // C-W; kill word
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

            case 'u':      // C-U; kill line
                while (term_curpos > 0) {
                    delch();
                }
                break;

            case 'h':      // C-H; kill char
                delch();
                break;

            case 'c':      // C-C; keyboard interrupt
                puts("^C");
                term_read_done = 1;
                term_buf_count = 0;
                break;

            case 'm':      // C-M / C-J; Return
                term_read_done = 1;
                break;
        }
    } else if (key.modifiers == MOD_ALT) { // An alt'd character
        switch (key.key) {
            case 'b':      // M-B; word back
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

            case 'f':      // M-F; word forward
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
    // Use control characters to handle the following keys
    } else if (key.key == KEY_ENTER) {
        term_key_handler((key_t) C('m'));
    } else if (key.key == KEY_BACK) {
        term_key_handler((key_t) C('h'));
    } else if (key.key == KEY_LEFT) {
        term_key_handler((key_t) C('b'));
    } else if (key.key == KEY_RIGHT) {
        term_key_handler((key_t) C('f'));
    } else if (key.key >= KEY_F1 && key.key <= KEY_F12) {
        // STUB!
        printf("F%d", key.key - KEY_F1);
    } else {
        addch(key.key);
    }
}

// Add a character to the line buf after the current cursor position
void addch(uint8_t ch) {
    if (term_buf_count < TERM_BUF_SIZE) {
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

// delete a character from the line buf before the current cursor position
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
    outb(0x0A, 0x3D4); outb(0x00, 0x3D5);   // Enable cursor; cursor scanline start at 0
    outb(0x0B, 0x3D4); outb(0x0F, 0x3D5);   // No cursor skew; cursor scanline ends at 15 => blocky cursors
    sti();
}
