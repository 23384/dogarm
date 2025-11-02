#ifndef DISASM_H
#define DISASM_H

#include "instr.h"
#include <stddef.h>
#include <stdint.h>

uint32_t disasm_read_u32_le(const uint8_t *buf);

int disasm_instruction(const uint8_t *buf, uint32_t address, instruction_t *instr, char *output, size_t outsize);

#endif

