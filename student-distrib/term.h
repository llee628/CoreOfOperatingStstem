#ifndef _TERM_H_
#define _TERM_H_

#include "types.h"

void term_key_handler(uint8_t ch);
void init_term();
int32_t term_write(const void* buf, int32_t nbytes);
int32_t term_read(void* buf, int32_t nbytes);
int32_t term_open();
int32_t term_close();

#endif
