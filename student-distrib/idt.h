#ifndef _IDT_H
#define _IDT_H

#define SYSCALL_IDX     0x80

// Interrupt indexes
#define KB_INT			0x21
#define SLAVE_PIC_INT	0x22
#define RTC_INT 		0x28

#ifndef ASM

/* Prints out the exception type that was thrown */
void general_exceptions_handler(char* exception);

/* 19 specific exception handlers that specify the exact exeption type
* functions names are based of the mnemonic for each exception specified
* on page 5-3 of the IA-32 manual
* https://courses.engr.illinois.edu/ece391/sp2019/secure/references/IA32-ref-manual-vol-3.pdf */
void de(void);
void db(void);
void nmi(void);
void bp(void);
void of(void);
void br(void);
void ud(void);
void nm(void);
void cso(void);
void df(void);
void ts(void);
void np(void);
void ss(void);
void gp(void);
void pf(void);
void mf(void);
void ac(void);
void mc(void);
void xf(void);
void unreachable(void);

void syscall_temp(void);

/* initializes the idt array */
extern void idt_init(void);

#endif /* ASM */

#endif
