#ifndef _TERM_H_
#define _TERM_H_

#include "types.h"

#define TERM_BUF_SIZE 128

uint8_t term_buf[TERM_BUF_SIZE];
uint8_t term_buf_count;
void term_key_handler(uint8_t ch);
void init_term();
void addch(uint8_t ch);
void delch();

#endif
