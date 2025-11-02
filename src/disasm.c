#include "disasm.h"
#include "decode.h"
#include <string.h>

uint32_t disasm_read_u32_le(const uint8_t *buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

int disasm_instruction(const uint8_t *buf, uint32_t address, instruction_t *instr, char *output, size_t outsize) {
    if (!buf || !instr || !output) {
        return -1;
    }
    
    instr->raw = disasm_read_u32_le(buf);
    instr->address = address;
    instr->cond = (instr->raw >> 28) & 0xf;
    instr->type = decode_instruction_type(instr->raw);
    
    return decode_instruction(instr, output, outsize);
}

