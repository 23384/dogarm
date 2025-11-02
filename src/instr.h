#ifndef INSTR_H
#define INSTR_H

#include <stdint.h>

typedef enum {
    INSTR_TYPE_UNDEFINED,
    INSTR_TYPE_DATA_PROC,
    INSTR_TYPE_MULTIPLY,
    INSTR_TYPE_MULTIPLY_LONG,
    INSTR_TYPE_SINGLE_DATA_TRANSFER,
    INSTR_TYPE_HALFWORD_DATA_TRANSFER,
    INSTR_TYPE_BLOCK_DATA_TRANSFER,
    INSTR_TYPE_BRANCH,
    INSTR_TYPE_SOFTWARE_INTERRUPT,
    INSTR_TYPE_COPROCESSOR,
    INSTR_TYPE_SWAP,
    INSTR_TYPE_PSR_TRANSFER
} instr_type_t;

typedef struct {
    uint32_t raw;
    uint32_t address;
    instr_type_t type;
    uint32_t cond;
} instruction_t;

#endif

