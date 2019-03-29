#ifndef _TERM_H_
#define _TERM_H_

#include "types.h"
#include "kb.h"
#include "task.h"

file_ops_table_t term_file_ops_table;

void term_key_handler(key_t key);
void init_term();
int32_t term_write(const void* buf, int32_t nbytes);
int32_t term_read(void* buf, int32_t nbytes);
int32_t term_open(const uint8_t *filename);
int32_t term_close();

#endif
