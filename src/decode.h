#ifndef DECODE_H
#define DECODE_H

#include "instr.h"
#include <stddef.h>

instr_type_t decode_instruction_type(uint32_t instr);

int decode_instruction(const instruction_t *instr, char *output, size_t outsize);

#endif

