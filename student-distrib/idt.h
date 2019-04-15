#ifndef _IDT_H
#define _IDT_H

#include "types.h"

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
void df(int32_t errorcode);
void ts(int32_t errorcode);
void np(int32_t errorcode);
void ss(int32_t errorcode);
void gp(int32_t errorcode);
void pf(int32_t errorcode);
void mf(void);
void ac(int32_t errorcode);
void mc(void);
void xf(void);
void unreachable(void);
void exception_handler(uint32_t irq_num, uint32_t errorcode);

void syscall_temp(void);

/* initializes the idt array */
extern void idt_init(void);

// Exception asm handlers
extern int _de_isr(void);
extern int _db_isr(void);
extern int _nmi_isr(void);
extern int _bp_isr(void);
extern int _of_isr(void);
extern int _br_isr(void);
extern int _ud_isr(void);
extern int _nm_isr(void);
extern int _df_isr(void);
extern int _cso_isr(void);
extern int _ts_isr(void);
extern int _np_isr(void);
extern int _ss_isr(void);
extern int _gp_isr(void);
extern int _pf_isr(void);
extern int _db_isr(void);
extern int _mf_isr(void);
extern int _ac_isr(void);
extern int _mc_isr(void);
extern int _xf_isr(void);

// External interrupt asm handlers
extern int _pti_isr(void);
extern int _kb_isr(void);
extern int _cas_isr(void);
extern int _com2_isr(void);
extern int _com1_isr(void);
extern int _lpt2_isr(void);
extern int _flop_isr(void);
extern int _lpt1_isr(void);
extern int _rtc_isr(void);
extern int _free1_isr(void);
extern int _free2_isr(void);
extern int _free3_isr(void);
extern int _ps2m_isr(void);
extern int _fpu_isr(void);
extern int _hd1_isr(void);
extern int _hd2_isr(void);

extern int _syscall_isr(void);

#endif /* ASM */

#endif
