#ifndef _KB_H_
#define _KB_H_

// Number of printble keys
#define PRINT_KEY_NUM 0x3A

void init_kb(void);
void kb_isr(void);

#endif
