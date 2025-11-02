#ifndef OPERAND_H
#define OPERAND_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    SHIFT_LSL = 0,
    SHIFT_LSR = 1,
    SHIFT_ASR = 2,
    SHIFT_ROR = 3
} shift_type_t;

typedef struct {
    enum {
        OPERAND_REGISTER,
        OPERAND_IMMEDIATE,
        OPERAND_REGISTER_SHIFTED,
        OPERAND_REGISTER_REGISTER_SHIFTED
    } type;
    
    union {
        struct {
            uint32_t reg;
        } reg;
        
        struct {
            uint32_t value;
        } imm;
        
        struct {
            uint32_t reg;
            shift_type_t shift;
            uint32_t amount;
        } reg_shifted;
        
        struct {
            uint32_t reg;
            shift_type_t shift;
            uint32_t shift_reg;
        } reg_reg_shifted;
    } data;
} operand_t;

int operand_format(const operand_t *op, char *buf, size_t bufsize);

operand_t operand_parse_imm(uint32_t instr);

operand_t operand_parse_reg(uint32_t instr);

#endif

