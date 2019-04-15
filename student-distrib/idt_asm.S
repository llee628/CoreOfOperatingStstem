# vim:ts=4 noexpandtab

#define ASM     1

#include "idt.h"
#include "rtc.h"

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

EXCEPTION_JMP_TAB:
    .long de
    .long db
    .long nmi
    .long bp
    .long of
    .long br
    .long ud
    .long nm
    .long df
    .long cso
    .long ts
    .long np
    .long ss
    .long gp
    .long pf
    .long mf
    .long ac
    .long mc
    .long xf

# Exception 1st level handler
# See IA-32 Manual p.145 for error code presence
_de_isr:
    push $0
    push $0
    jmp _exception_isr

_db_isr:
    push $0
    push $1
    jmp _exception_isr

_nmi_isr:
    push $0
    push $2
    jmp _exception_isr

_bp_isr:
    push $0
    push $3
    jmp _exception_isr

_of_isr:
    push $0
    push $4
    jmp _exception_isr

_br_isr:
    push $0
    push $5
    jmp _exception_isr

_ud_isr:
    push $0
    push $6
    jmp _exception_isr

_nm_isr:
    push $0
    push $7
    jmp _exception_isr

_df_isr:
    push $8
    jmp _exception_isr

_cso_isr:
    push $0
    push $9
    jmp _exception_isr

_ts_isr:
    push $10
    jmp _exception_isr

_np_isr:
    push $11
    jmp _exception_isr

_ss_isr:
    push $12
    jmp _exception_isr

_gp_isr:
    push $13
    jmp _exception_isr

_pf_isr:
    push $14
    jmp _exception_isr

_mf_isr:
    push $0
    push $15
    jmp _exception_isr

_ac_isr:
    push $16
    jmp _exception_isr

_mc_isr:
    push $0
    push $17
    jmp _exception_isr

_xf_isr:
    push $0
    push $18
    jmp _exception_isr

_exception_isr:
    pusha
    push 36(%esp)    // Copy the errorcode
    mov 36(%esp), %eax     // IRQ num
    mov EXCEPTION_JMP_TAB(, %eax, 4), %eax
    call *%eax
    mov %eax, 32(%esp)    // Store eax back to the stored regs
    jmp _exception_isr__return

_exception_isr__error:
    mov $-1, 32(%esp)

_exception_isr__return:
    add $4, %esp     // Pop copied errorcode
    popa
    add $8, %esp     // Pop original errorcode and IRQ
    iret

SYSCALL_JMP_TAB:
    .long syscall_halt
    .long syscall_execute
    .long syscall_read
    .long syscall_write
    .long syscall_open
    .long syscall_close
    .long syscall_getargs
    .long syscall_vidmap
    .long syscall_set_handler
    .long syscall_sigreturn
    .long syscall_malloc

# Syscall
_syscall_isr:
    pusha
    push %edx
    push %ecx
    push %ebx
    cmp $1, %eax
    jl _syscall_isr__error
    cmp $11, %eax
    jg _syscall_isr__error
    sub $1, %eax
    mov SYSCALL_JMP_TAB(, %eax, 4), %eax
    call *%eax
    add $12, %esp
    mov %eax, 28(%esp)
    popa
    iret

_syscall_isr__error:
    mov $-1, %eax
    add $12, %esp
    mov %eax, 28(%esp)
    popa
    iret

# Interrupt 1st level handlers
PIC_ISR_jmp_tab:
    .long db                // 0     Programmable Interrupt Timer Interrupt
    .long kb_isr            // 1     Keyboard Interrupt
    .long unreachable       // 2     Cascade (used internally by the two PICs. never raised)
    .long db                // 3     COM2 (if enabled)
    .long db                // 4     COM1 (if enabled)
    .long db                // 5     LPT2 (if enabled)
    .long db                // 6     Floppy Disk
    .long db                // 7     LPT1 / Unreliable "spurious" interrupt (usually)
    .long rtc_isr           // 8     CMOS real-time clock (if enabled)
    .long db                // 9     Free for peripherals / legacy SCSI / NIC
    .long db                // 10     Free for peripherals / SCSI / NIC
    .long db                // 11     Free for peripherals / SCSI / NIC
    .long db                // 12     PS2 Mouse
    .long db                // 13     FPU / Coprocessor / Inter-processor
    .long db                // 14     Primary ATA Hard Disk
    .long db                // 15     Secondary ATA Hard Disk

PIC_ISR:
    pusha
    mov 32(%esp), %eax
    mov PIC_ISR_jmp_tab(, %eax, 4), %eax
    call *%eax
    cmp $8, %eax
    jge PIC_ISR__slave_EOI
    mov $0x20, %al
    outb %al, $0x20
    jmp PIC_ISR__end

PIC_ISR__slave_EOI:
    mov $0x20, %al
    outb %al, $0x20
    outb %al, $0xA0

PIC_ISR__end:
    popa
    add $4, %esp     // Pop IRQ num
    iret



_pti_isr:    // 0     Programmable Interrupt Timer Interrupt
    push $0
    jmp PIC_ISR

_kb_isr:    // 1     Keyboard Interrupt
    push $1
    jmp PIC_ISR

_cas_isr:    // 2     Cascade (used internally by the two PICs. never raised)
    push $2
    jmp PIC_ISR

_com2_isr:    // 3     COM2 (if enabled)
    push $3
    jmp PIC_ISR

_com1_isr:    // 4     COM1 (if enabled)
    push $4
    jmp PIC_ISR

_lpt2_isr:    // 5     LPT2 (if enabled)
    push $5
    jmp PIC_ISR

_flop_isr:    // 6     Floppy Disk
    push $6
    jmp PIC_ISR

_lpt1_isr:    // 7     LPT1 / Unreliable "spurious" interrupt (usually)
    push $7
    jmp PIC_ISR

_rtc_isr:    // 8     CMOS real-time clock (if enabled)
    push $8
    jmp PIC_ISR

_free1_isr:    // 9     Free for peripherals / legacy SCSI / NIC
    push $9
    jmp PIC_ISR

_free2_isr:    // 10     Free for peripherals / SCSI / NIC
    push $10
    jmp PIC_ISR

_free3_isr:    // 11     Free for peripherals / SCSI / NIC
    push $11
    jmp PIC_ISR

_ps2m_isr:    // 12     PS2 Mouse
    push $12
    jmp PIC_ISR

_fpu_isr:    // 13     FPU / Coprocessor / Inter-processor
    push $13
    jmp PIC_ISR

_hd1_isr:    // 14     Primary ATA Hard Disk
    push $14
    jmp PIC_ISR

_hd2_isr:    // 15     Secondary ATA Hard Disk
    push $15
    jmp PIC_ISR

.globl do_syscall
do_syscall:
    /* pushl    %ebx */
    movl    4(%esp),%eax
    movl    8(%esp),%ebx
    movl    12(%esp),%ecx
    movl    16(%esp),%edx
    int    $0x80
    /* popl    %ebx */
    ret

