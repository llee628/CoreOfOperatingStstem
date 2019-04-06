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
// Echo the character to the screen
uint8_t term_echo = 1;
// Non-canonical operation; buffer the inputs
uint8_t term_nocanon = 1;

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

int32_t term_read(void* buf, int32_t nbytes, FILE *file) {
    if (!buf) {
        return -1;
    }
    if (term_nocanon) {
        sti();
        while (!term_read_done) {
            asm volatile ("hlt");
        }
        cli();
        if (term_echo) {
            putc('\n');
        }
        term_buf[term_buf_count++] = '\n';
        int copy_count = nbytes < term_buf_count ? nbytes : term_buf_count;
        memcpy(buf, term_buf, copy_count);
        term_buf_count = 0;
        term_read_done = 0;
        term_curpos = 0;
        return copy_count;
    } else {
        while (!term_buf_count) {
            asm volatile ("hlt");
        }
        memcpy(buf, term_buf, 1);
        term_curpos = 1;
        delch();
        return 1;
    }
}

void esc_funcs(uint8_t f, uint8_t *args, uint8_t arg_len) {
    uint8_t setting = 0;
    switch (f) {
        // Foreground color
        case 'f':
            if (arg_len) {
                setattr((getattr() & 0xF0) | (args[0] & 0x0F));
            }
            break;

        // Background color
        case 'b':
            if (arg_len) {
                setattr((getattr() & 0x0F) | (args[0] & 0xF0));
            }
            break;

        // Set cursor position
        case 'p':
            if (arg_len > 1) {
                setpos(args[0] - 1, args[1] - 1);
            }
            break;

        case 's': case 'S':
            // Set the value if 's'
            setting = f == 's';
            if (arg_len) {
                switch (args[0]) {
                    case 1: term_echo = setting; break;
                    case 2: term_nocanon = setting; break;
                }
            }
            break;
    }
}

// Function to parse the escape sequence for `term_write'; returns 1 when the
// literal character should be put on screen
int8_t esc_parse(uint8_t c) {
    static esc_state_t state = IDLE;
    // Buffer for each argument
    static int8_t buf[4];
    static uint8_t buf_len = 0;
    // All the arguments
    static uint8_t args[4];
    static uint8_t arg_len = 0;
    // Unconditional reset; start a new sequence whenever an escape is
    // encountered, regardless of current progress
    if (c == 0x1b) {
        state = A_ESCAPE;
        return 0;
    }
    if (state == A_ESCAPE && c == '[') {
        state = A_BRACKET;
        return 0;
    }
    if (state == A_BRACKET) {
        arg_len = 0;
        buf_len = 0;
        state = ARG_PARSE;
    }

    if (state == ARG_PARSE) {
        if (isalpha(c)) {
            if (buf_len < 3) {
                buf[buf_len++] = c;
            }
            return 0;
        } else if (c == ';') {
            if (arg_len < 3) {
                args[arg_len++] = atoi(buf, 10);
            }
            return 0;
        } else {
            esc_funcs(c, args, arg_len);
            state = IDLE;
            return 0;
        }
    }
    return 1;
}

int32_t term_write(const void* buf, int32_t nbytes, FILE *file) {
    int i;
    for (i = 0; i < nbytes; i ++) {
        uint8_t c = ((char * ) buf)[i];
        if (esc_parse(c)) {
            putc(c);
        }
    }
    return nbytes;
}

void term_key_handler(key_t key) {
    if (!term_nocanon) {    // Canonical mode
        term_curpos = term_buf_count;
        addch(key.key);
        return;
    }

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
        if (term_echo) {
            putc(ch);
            for (i = term_curpos; i < term_buf_count; i ++) {
                putc(term_buf[i]);
            }
            for (i = term_buf_count - term_curpos; i > 0; i --) {
                back();
            }
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
