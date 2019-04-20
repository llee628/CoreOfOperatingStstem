#include "term.h"
#include "kb.h"
#include "lib.h"
#include "syscall.h"

term_t terms[TERM_NUM];
uint8_t cur_term_ind = 0;
term_t *cur_term = terms;
static uint8_t attr = DEF_ATTR;
static uint8_t* video_mem = (uint8_t *)VIDEO;

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
int32_t term_open(const int8_t *filename, FILE *file) {
    return 0;
}

int32_t term_close() {
    return 0;
}

int32_t term_read(int8_t* buf, uint32_t nbytes, FILE *file) {
    if (!buf) {
        return -1;
    }
    if (cur_term->term_canon) {
        sti();
        while (!cur_term->term_buf_count) {
            asm volatile ("hlt");
        }
        cli();
        memcpy(buf, cur_term->term_buf, 1);
        cur_term->term_curpos = 1;
        delch();
        return 1;
    } else {
        sti();
        while (!cur_term->term_read_done) {
            asm volatile ("hlt");
        }
        cli();
        if (!cur_term->term_noecho) {
            putc('\n');
        }
        cur_term->term_buf[cur_term->term_buf_count++] = '\n';
        int copy_count = nbytes < cur_term->term_buf_count ? nbytes : cur_term->term_buf_count;
        memcpy(buf, cur_term->term_buf, copy_count);
        cur_term->term_buf_count = 0;
        cur_term->term_read_done = 0;
        cur_term->term_curpos = 0;
        return copy_count;
    }
}

void esc_funcs(uint8_t f, uint8_t *args, uint8_t arg_len) {
    uint8_t setting = 0;
    switch (f) {
        // Foreground color
        case 'f':
            if (arg_len) {
                setattr((getattr() & 0xF0) | (args[0] & 0x0F));
            } else {
                setattr((getattr() & 0xF0) | (DEF_ATTR & 0x0F));
            }
            break;

        // Background color
        case 'b':
            if (arg_len) {
                setattr((getattr() & 0x0F) | ((args[0] & 0x0F) << 4));
            } else {
                setattr((getattr() & 0x0F) | (DEF_ATTR & 0xF0));
            }
            break;

        // Set cursor position
        case 'p':
            if (arg_len > 1) {
                setpos(args[1] - 1, args[0] - 1);
            }
            break;

        // Save cursor position
        case 'e':
            if (!arg_len) {
                cur_term->cur_x_store = getposx();
                cur_term->cur_y_store = getposy();
            }
            break;

        // Restore cursor position
        case 'r':
            if (!arg_len) {
                setpos(cur_term->cur_x_store, cur_term->cur_y_store);
            }
            break;

        // Clear screen
        case 'c':
            if (!arg_len) {
                clear();
            }
            break;

        case 's': case 'S':
            // Set the value if 's'
            setting = f == 's';
            if (arg_len) {
                switch (args[0]) {
                    case 1: cur_term->term_noecho = setting; break;
                    case 2: cur_term->term_canon = setting; break;
                    case 3:
                        if (setting) {
                            // Cursor scanline ends at 0; no cursor
                            outb(0x0B, 0x3D4); outb(0x00, 0x3D5);
                        } else {
                            // Cursor scanline ends at 15; back to blocky cursor
                            outb(0x0B, 0x3D4); outb(0x0F, 0x3D5);
                        }
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
        if (isnum(c)) {
            if (buf_len < 3) {
                buf[buf_len++] = c;
            }
            return 0;
        } else if (c == ';') {
            if (arg_len < 3) {
                buf[buf_len] = 0;
                args[arg_len++] = atoi(buf, 10);
            }
            buf_len = 0;
            return 0;
        } else {
            esc_funcs(c, args, arg_len);
            state = IDLE;
            buf_len = arg_len = 0;
            return 0;
        }
    }
    return 1;
}

int32_t term_write(const int8_t* buf, uint32_t nbytes, FILE *file) {
    int i;
    for (i = 0; i < nbytes; i ++) {
        uint8_t c = ((char * ) buf)[i];
        if (esc_parse(c)) {
            putc(c);
        }
    }
    return nbytes;
}

void switch_term(uint8_t ind) {
    if (ind == cur_term_ind) {
        return;
    }

    // Save old terminal
    PCB_t *task_pcb = get_cur_pcb();
    cur_term->cur_pid = task_pcb->pid;
    memcpy(cur_term->video_buffer, video_mem, VID_MEM_SIZE);
    cur_term->video_mem = cur_term->video_buffer;

    // Put on new terminal
    cur_term_ind = ind;
    cur_term = &terms[ind];
    cur_term->video_mem = video_mem;

    if (!cur_term->cur_pid) {
        /* cur_term->used = 1; */
        clear();
        _syscall_execute("shell", ind);       // TODO
    } else {
        memcpy(video_mem, cur_term->video_buffer, VID_MEM_SIZE);
        uint8_t target_pid = cur_term->cur_pid;
        PCB_t *target_pcb = (PCB_t *) TASK_KSTACK_TOP(target_pid);

        uint32_t target_ebp = (uint32_t) target_pcb->ebp;
        uint32_t target_esp = (uint32_t) target_pcb->esp;
        asm volatile (
            "movl %0, %%eax;"
            "movl %1, %%ecx;"
            "movl %%eax, %%ebp;"
            "movl %%ecx, %%esp;"
            "sti;"
            /* "jmp 1f;" */
            :
            : "m" (target_ebp), "m" (target_esp)
            : "%eax", "%ecx"
        );
        return;
    }
}

void term_key_handler(key_t key) {
    if (cur_term->term_canon) {    // Canonical mode
        cur_term->term_curpos = cur_term->term_buf_count;
        addch(key.key);
        return;
    }

    int i;
    if (key.modifiers == MOD_CTRL) {     // Non-printable; probably a control character
        switch (key.key) {
            case 'l':      // C-L; clear
                for (i = 0; i < cur_term->term_curpos; i ++) {
                    back();
                }
                for (i = getposy(); i > 0; i --) {
                    scroll();
                }
                for (i = 0; i < cur_term->term_curpos; i ++) {
                    forward();
                }
                break;

            case 'b':      // C-B; char back
                if (cur_term->term_curpos > 0) {
                    cur_term->term_curpos --;
                    back();
                }
                break;

            case 'f':      // C-F; char forward
                if (cur_term->term_curpos < cur_term->term_buf_count) {
                    cur_term->term_curpos ++;
                    forward();
                }
                break;

            case 'a':      // C-A; start of line
                while (cur_term->term_curpos > 0) {
                    cur_term->term_curpos --;
                    back();
                }
                break;

            case 'e':      // C-E; end of line
                while (cur_term->term_curpos < cur_term->term_buf_count) {
                    cur_term->term_curpos ++;
                    forward();
                }
                break;

            case 'w':      // C-W; kill word
                if (!cur_term->term_curpos) {
                    break;
                }

                if (isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                    while (cur_term->term_curpos && isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                        delch();
                    }
                } else {
                    while (cur_term->term_curpos && !isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                        delch();
                    }
                }
                break;

            case 'u':      // C-U; kill line
                while (cur_term->term_curpos > 0) {
                    delch();
                }
                break;

            case 'h':      // C-H; kill char
                delch();
                break;

            case 'c':      // C-C; keyboard interrupt
                puts("^C");
                PCB_t *task_pcb = get_cur_pcb();
                task_pcb->signals |= SIG_FLAG(SIG_KB_INT);
                /* term_read_done = 1; */
                /* term_buf_count = 0; */
                return;

            case 'm':      // C-M / C-J; Return
                cur_term->term_read_done = 1;
                break;
        }
    } else if (key.modifiers == MOD_ALT) { // An alt'd character
        switch (key.key) {
            case KEY_F1: case KEY_F2: case KEY_F3:
                switch_term(key.key - KEY_F1);
                break;

            case 'b':      // M-B; word back
                if (!cur_term->term_curpos) {
                    break;
                }

                if (isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                    while (cur_term->term_curpos && isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                        cur_term->term_curpos --;
                        back();
                    }
                } else {
                    while (cur_term->term_curpos && !isalnum(cur_term->term_buf[cur_term->term_curpos - 1])) {
                        cur_term->term_curpos --;
                        back();
                    }
                }
                break;

            case 'f':      // M-F; word forward
                if (cur_term->term_curpos >= cur_term->term_buf_count) {
                    break;
                }

                if (isalnum(cur_term->term_buf[cur_term->term_curpos])) {
                    while ((cur_term->term_curpos < cur_term->term_buf_count)
                            && isalnum(cur_term->term_buf[cur_term->term_curpos])) {
                        cur_term->term_curpos ++;
                        forward();
                    }
                } else {
                    while ((cur_term->term_curpos < cur_term->term_buf_count)
                            && !isalnum(cur_term->term_buf[cur_term->term_curpos])) {
                        cur_term->term_curpos ++;
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
    if (cur_term->term_buf_count < TERM_BUF_SIZE) {
        int i;
        for (i = cur_term->term_buf_count; i > cur_term->term_curpos; i --) {
            cur_term->term_buf[i] = cur_term->term_buf[i - 1];
        }
        cur_term->term_buf[cur_term->term_curpos++] = ch;
        cur_term->term_buf_count ++;
        if (!cur_term->term_noecho) {
            putc(ch);
            for (i = cur_term->term_curpos; i < cur_term->term_buf_count; i ++) {
                putc(cur_term->term_buf[i]);
            }
            for (i = cur_term->term_buf_count - cur_term->term_curpos; i > 0; i --) {
                back();
            }
        }
    }
}

// delete a character from the line buf before the current cursor position
void delch() {
    if (cur_term->term_curpos > 0) {
        int i;
        for (i = cur_term->term_curpos - 1; i < cur_term->term_buf_count - 1; i ++) {
            cur_term->term_buf[i] = cur_term->term_buf[i + 1];
        }
        cur_term->term_curpos --;
        cur_term->term_buf_count --;
        if (!cur_term->term_canon) {
            back();
            int old_x = getposx(), old_y = getposy();
            for (i = cur_term->term_curpos; i < cur_term->term_buf_count; i ++) {
                putc(cur_term->term_buf[i]);
            }
            putc(' ');
            setpos(old_x, old_y);
        }
    }
}


void init_term() {
    cli();
    outb(0x0A, 0x3D4); outb(0x00, 0x3D5);   // Enable cursor; cursor scanline start at 0
    outb(0x0B, 0x3D4); outb(0x0F, 0x3D5);   // No cursor skew; cursor scanline ends at 15 => blocky cursors
    
    /* init_proc("shell", 0); */
    /* init_proc("shell", 1); */
    /* init_proc("shell", 2); */

    sti();
}

int getposx() {
	return cur_term->cur_x;
}

int getposy() {
	return cur_term->cur_y;
}

/* void clear();
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear() {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = attr;
    }
}

/* void setpos(int x, int y);
 * Inputs: int x, y: position of cursor to set to
 * Return Value: void
 *  Function: Set the cursor position */
void setpos(int x, int y) {
	if (x < 0) {
		x = 0;
	} else if (x >= NUM_COLS) {
		x = NUM_COLS - 1;
	}
	if (y < 0) {
		y = 0;
	} else if (y >= NUM_ROWS) {
		y = NUM_ROWS - 1;
	}

	cur_term->cur_x = x;
	cur_term->cur_y = y;

	// Set VGA cursor position
	uint16_t curpos = cur_term->cur_x + cur_term->cur_y * NUM_COLS;
	outb(0x0F, 0x3D4);
	outb(curpos & 0xFF, 0x3D5);
	outb(0x0E, 0x3D4);
	outb((curpos >> 8) & 0xFF, 0x3D5);
}

/* void setattr(uint8_t _attr);
 * Inputs: uint_8 _attr: Attribute to set
 * Return Value: void
 *  Function: Set the attribute of following prints to _attr */
void setattr(uint8_t _attr) {
	attr = _attr;
}

uint8_t getattr() {
	return attr;
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *   Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void get_vid_char(int x, int y);
 * Inputs: int x, y: position of character to get
 * Return Value: void
 *  Function: get a character located at the input positions */
uint8_t get_vid_char(int x, int y) {
	if (x < 0 || x >= NUM_COLS || y < 0 || y >= NUM_ROWS) {
		return 0;
	}

	return video_mem[(NUM_COLS * y + x) << 1];
}

/* void set_vid_char(int x, int y);
 * Inputs: int x, y: position of character to set
 * Return Value: void
 *  Function: set a character located at the input positions */
void set_vid_char(int x, int y, uint8_t ch) {
	if (x < 0 || x >= NUM_COLS || y < 0 || y >= NUM_ROWS) {
		return;
	}

	video_mem[(NUM_COLS * y + x) << 1] = ch;
	video_mem[((NUM_COLS * y + x) << 1) + 1] = attr;
}

void scroll() {
	cli();
	int x, y;
	uint8_t old_attr = attr;
	for (y = 0; y < NUM_ROWS - 1; y ++) {
		for (x = 0; x < NUM_COLS; x ++) {
			// We want to copy the attributes as well
			setattr(video_mem[((NUM_COLS * (y + 1) + x) << 1) + 1]);
			set_vid_char(x, y, get_vid_char(x, y + 1));
		}
	}
	setattr(DEF_ATTR);
	for (x = 0; x < NUM_COLS; x ++) {
		set_vid_char(x, NUM_ROWS - 1, ' ');
	}
	setpos(cur_term->cur_x, cur_term->cur_y - 1);
	setattr(old_attr);
	/* sti(); */
}

void back() {
	cur_term->cur_x --;
	if (cur_term->cur_x < 0) {
		cur_term->cur_x += NUM_COLS;
		cur_term->cur_y --;
	}
	setpos(cur_term->cur_x, cur_term->cur_y);
}

void forward() {
	cur_term->cur_x ++;
	if (cur_term->cur_x >= NUM_COLS) {
		cur_term->cur_x -= NUM_COLS;
		cur_term->cur_y ++;
	}
	setpos(cur_term->cur_x, cur_term->cur_y);
}

void putc(uint8_t c) {
	switch (c) {
		case '\n':
			cur_term->cur_y ++;
			cur_term->cur_x = 0;
			if (cur_term->cur_y >= NUM_ROWS) {
				scroll();
				cur_term->cur_y = NUM_ROWS - 1;
			}
			break;

		case '\r':
			cur_term->cur_x = 0;
			break;

		case '\b':
			back();
			set_vid_char(cur_term->cur_x, cur_term->cur_y, ' ');
			break;

		default:
			set_vid_char(cur_term->cur_x, cur_term->cur_y, c);
			cur_term->cur_x++;
			if (cur_term->cur_x >= NUM_COLS) {
				cur_term->cur_x -= NUM_COLS;
				cur_term->cur_y ++;
			}
			if (cur_term->cur_y >= NUM_ROWS) {
				scroll();
				cur_term->cur_y = NUM_ROWS - 1;
			}
			break;
	}
	setpos(cur_term->cur_x, cur_term->cur_y);
}
