#ifndef _TERM_H_
#define _TERM_H_

#include "types.h"
#include "kb.h"
#include "task.h"

typedef enum {
    IDLE,
    A_ESCAPE,
    A_BRACKET,
    ARG_PARSE,
    PARTIAL_FINISH,
} esc_state_t;

file_ops_table_t term_file_ops_table;

void term_key_handler(key_t key);
void init_term();
int32_t term_write(const int8_t* buf, uint32_t nbytes, FILE *file);
int32_t term_read(int8_t* buf, uint32_t nbytes, FILE *file);
int32_t term_open(const int8_t *filename, FILE *file);
int32_t term_close();

#endif
