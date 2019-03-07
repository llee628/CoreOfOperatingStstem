#include "idt.h"
#include "x86_desc.h"
#include "lib.h"


/* Function: general_exceptions_handler;
 * Inputs: exception - the string of which exception to print
 * Return Value: none
 * Description: Prints out the exception being thrown and stops the OS from
 * being interrupted by anything else
 */
void general_exceptions_handler(char* exception){
  printf("%s \n", exception);
  while(1);
}

/* Functions: mnemonics (gotten from IA-32 manual) for the exception;
 * Inputs: none
 * Return Value: none
 * Description: Specific Handlers for each unique exception as described in the IA-32 manual
 *              They all pretty much act the same just different strings to pass
 * *Note: Much too lazy to write function headers for all 18 since they do the
 *        exact same thing but are necessary to be seperate
 */
void de(void){ general_exceptions_handler("DIVIDE BY ZERO ERROR"); }
void db(void){ general_exceptions_handler("INTEL RESERVED"); }
void nmi(void){ general_exceptions_handler("NMI INTERRUPT"); }
void bp(void){ general_exceptions_handler("BREAKPOINT"); }
void of(void){ general_exceptions_handler("OVERFLOW"); }
void br(void){ general_exceptions_handler("BOUND RANGE EXCEEDED"); }
void ud(void){ general_exceptions_handler("INVALID OPCODE (UNDEFINED OPCODE)"); }
void nm(void){ general_exceptions_handler("DEVICE NOT AVAILABLE (NO MATH COPROCESSOR)"); }
void df(void){ general_exceptions_handler("DOUBLE FAULT"); }
void cso(void){ general_exceptions_handler("COPROCESSOR SEGMENT OVERRUN"); }
void ts(void){ general_exceptions_handler("INVALID TSS"); }
void np(void){ general_exceptions_handler("SEGMENT NOT PRESENT"); }
void ss(void){ general_exceptions_handler("STACK-SEGMENT FAULT"); }
void gp(void){ general_exceptions_handler("GENERAL PROTECTION"); }
void pf(void){ general_exceptions_handler("PAGE FAULT"); }
void mf(void){ general_exceptions_handler("X87 FPU FLOATING-POINT ERROR"); }
void ac(void){ general_exceptions_handler("ALIGNMENT CHECK"); }
void mc(void){ general_exceptions_handler("MACHINE CHECK"); }
void xf(void){ general_exceptions_handler("SIMD FLOATING-POINT EXCEPTION"); }

/* temp function just to see if the index at 0x80 can be set*/
void syscall_temp(void){ printf("In syscall_temp\n"); }

/* Function creates everything as interrupt gates as recommended by descriptor doc
* "For simplicity,use interrupt gates for everything"
* https://courses.engr.illinois.edu/ece391/sp2019/secure/references/descriptors.pdf
* idt_desc_t union data is set according to the above doc as well
*/

/* Function: idt_int;
 * Inputs: void
 * Return Value: none
 * Description: Initializes the entire idt array
 */
void idt_init(void){
  // loop index for idt array
  int i;
  // initializes the entirety of the idt array
  // values are set according to the descriptor document
  // makes only interrupt gates
  for(i = 0; i < NUM_VEC; i++){
    idt[i].seg_selector = KERNEL_CS;
    idt[i].reserved3 = 0x0;
    idt[i].reserved2 = 0x1;
    idt[i].reserved1 = 0x1;
    idt[i].size = 0x1;
    idt[i].reserved0 = 0x0;
    idt[i].present = 0x1;
    if(i == SYSCALL_IDX){ idt[i].dpl = 0x3; }          // as instructed, the syscall privelege is set to 3
    else{ idt[i].dpl = 0x0; }                        // all other priveleges are initialized to 0

    // sets the Intel reserved exceptions from indices 20-31
    if( (i > 19) && (i < 32) ){ SET_IDT_ENTRY(idt[i], &db ); }
  }

  SET_IDT_ENTRY(idt[0], &de );                      // sets Divide by Zero exception
  SET_IDT_ENTRY(idt[1], &db );                      // sets Intel Reserved exception
  SET_IDT_ENTRY(idt[2], &nmi );                     // sets NMI interrupt exception
  SET_IDT_ENTRY(idt[3], &bp );                      // sets Breakpoint exception
  SET_IDT_ENTRY(idt[4], &of );                      // sets Overflow exception
  SET_IDT_ENTRY(idt[5], &br );                      // sets Bound Range exception
  SET_IDT_ENTRY(idt[6], &ud );                      // sets Invalid Opcode exception
  SET_IDT_ENTRY(idt[7], &nm );                      // sets Device Not Available exception
  SET_IDT_ENTRY(idt[8], &df );                      // sets Double Fault exception
  SET_IDT_ENTRY(idt[9], &cso );                     // sets Coprocessor Segment Overrun exception
  SET_IDT_ENTRY(idt[10], &ts );                     // sets Invalid TSS exception
  SET_IDT_ENTRY(idt[11], &np );                     // sets Segment Not Present exception
  SET_IDT_ENTRY(idt[12], &ss );                     // sets Stack-Segment Fault exception
  SET_IDT_ENTRY(idt[13], &gp );                     // sets General Protection excpetion
  SET_IDT_ENTRY(idt[14], &pf );                     // sets Page Fault exception
  SET_IDT_ENTRY(idt[15], &db );                     // sets the second Intel Reserved exception
  SET_IDT_ENTRY(idt[16], &mf );                     // sets x87 Floating Point Error exception
  SET_IDT_ENTRY(idt[17], &ac );                     // sets Alignment Check exception
  SET_IDT_ENTRY(idt[18], &mc );                     // sets Machine Check expception
  SET_IDT_ENTRY(idt[19], &xf );                     // sets SIMD Floating Point exception

  /* temp set up just to check if idt[syscall] can be non null */
  SET_IDT_ENTRY(idt[SYSCALL_IDX], &syscall_temp);

}
