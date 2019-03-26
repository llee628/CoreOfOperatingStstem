# vim:ts=4 noexpandtab

#define ASM     1

#include "idt.h"
#include "rtc.h"

.globl interrupt_handler
.globl _de_isr
.globl _db_isr
.globl _nmi_isr
.globl _bp_isr
.globl _of_isr
.globl _br_isr
.globl _ud_isr
.globl _nm_isr
.globl _df_isr
.globl _cso_isr
.globl _ts_isr
.globl _np_isr
.globl _ss_isr
.globl _gp_isr
.globl _pf_isr
.globl _mf_isr
.globl _ac_isr
.globl _mc_isr
.globl _xf_isr
.globl _pti_isr
.globl _kb_isr
.globl _cas_isr
.globl _com2_isr
.globl _com1_isr
.globl _lpt2_isr
.globl _flop_isr
.globl _lpt1_isr
.globl _rtc_isr
.globl _free1_isr
.globl _free2_isr
.globl _free3_isr
.globl _ps2m_isr
.globl _fpu_isr
.globl _hd1_isr
.globl _hd2_isr
.globl _syscall_isr

# Exception 1st level handler
# See IA-32 Manual p.145 for error code presence
_de_isr:
	pusha
	call de
	popa
	iret

_db_isr:
	pusha
	call db
	popa
	iret

_nmi_isr:
	pusha
	call nmi
	popa
	iret

_bp_isr:
	pusha
	call bp
	popa
	iret

_of_isr:
	pusha
	call of
	popa
	iret

_br_isr:
	pusha
	call br
	popa
	iret

_ud_isr:
	pusha
	call ud
	popa
	iret

_nm_isr:
	pusha
	call nm
	popa
	iret

_df_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call df
	popa
	iret

_cso_isr:
	pusha
	call cso
	popa
	iret

_ts_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call ts
	popa
	iret

_np_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call np
	popa
	iret

_ss_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call ss
	popa
	iret

_gp_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call gp
	popa
	iret

_pf_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call pf
	popa
	iret

_mf_isr:
	pusha
	call mf
	popa
	iret

_ac_isr:
	pusha
	pop %eax 	# Pop the error code for now
	call ac
	popa
	iret

_mc_isr:
	pusha
	call mc
	popa
	iret

_xf_isr:
	pusha
	call xf
	popa
	iret

# Syscall
_syscall_isr:
    /* pusha */
    push %edx
    push %ecx
    push %ebx
    push %eax
    call syscall_isr
    add $16, %esp
    /* popa */
    iret

# Interrupt 1st level handlers
PIC_ISR_jmp_tab:
    .long db                // 0 	Programmable Interrupt Timer Interrupt
    .long kb_isr            // 1 	Keyboard Interrupt
    .long unreachable       // 2 	Cascade (used internally by the two PICs. never raised)
    .long db                // 3 	COM2 (if enabled)
    .long db                // 4 	COM1 (if enabled)
    .long db                // 5 	LPT2 (if enabled)
    .long db                // 6 	Floppy Disk
    .long db                // 7 	LPT1 / Unreliable "spurious" interrupt (usually)
    .long rtc_isr           // 8 	CMOS real-time clock (if enabled)
    .long db                // 9 	Free for peripherals / legacy SCSI / NIC
    .long db                // 10 	Free for peripherals / SCSI / NIC
    .long db                // 11 	Free for peripherals / SCSI / NIC
    .long db                // 12 	PS2 Mouse
    .long db                // 13 	FPU / Coprocessor / Inter-processor
    .long db                // 14 	Primary ATA Hard Disk
    .long db                // 15 	Secondary ATA Hard Disk 

PIC_ISR:
    pop %eax        // IRQ num
    push %eax       // Save it again for later
    mov PIC_ISR_jmp_tab(, %eax, 4), %eax
    call *%eax
    pop %eax
    mov $8, %ebx
    cmp %ebx, %eax
    jge PIC_ISR__slave_EOI
    mov $0x20, %al
    outb %al, $0x20
    jmp PIC_ISR__end

PIC_ISR__slave_EOI:
    mov $0x20, %al
    outb %al, $0x20
    outb %al, $0xA0

PIC_ISR__end:
    iret



_pti_isr:	// 0 	Programmable Interrupt Timer Interrupt
	push $0
    jmp PIC_ISR

_kb_isr:	// 1 	Keyboard Interrupt
	push $1
    jmp PIC_ISR

_cas_isr:	// 2 	Cascade (used internally by the two PICs. never raised)
	push $2
    jmp PIC_ISR

_com2_isr:	// 3 	COM2 (if enabled)
	push $3
    jmp PIC_ISR

_com1_isr:	// 4 	COM1 (if enabled)
	push $4
    jmp PIC_ISR

_lpt2_isr:	// 5 	LPT2 (if enabled)
	push $5
    jmp PIC_ISR

_flop_isr:	// 6 	Floppy Disk
	push $6
    jmp PIC_ISR

_lpt1_isr:	// 7 	LPT1 / Unreliable "spurious" interrupt (usually)
	push $7
    jmp PIC_ISR

_rtc_isr:	// 8 	CMOS real-time clock (if enabled)
	push $8
    jmp PIC_ISR

_free1_isr:	// 9 	Free for peripherals / legacy SCSI / NIC
	push $9
    jmp PIC_ISR

_free2_isr:	// 10 	Free for peripherals / SCSI / NIC
	push $10
    jmp PIC_ISR

_free3_isr:	// 11 	Free for peripherals / SCSI / NIC
	push $11
    jmp PIC_ISR

_ps2m_isr:	// 12 	PS2 Mouse
	push $12
    jmp PIC_ISR

_fpu_isr:	// 13 	FPU / Coprocessor / Inter-processor
	push $13
    jmp PIC_ISR

_hd1_isr:	// 14 	Primary ATA Hard Disk
	push $14
    jmp PIC_ISR

_hd2_isr:	// 15 	Secondary ATA Hard Disk 
	push $15
    jmp PIC_ISR

.globl do_syscall
do_syscall:
    /* pushl	%ebx */
	movl	4(%esp),%eax
	movl	8(%esp),%ebx
	movl	12(%esp),%ecx
	movl	16(%esp),%edx
	int	$0x80
	/* popl	%ebx */
	ret

