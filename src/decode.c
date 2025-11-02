#include "decode.h"
#include "operand.h"
#include <stdio.h>
#include <string.h>

#define ARM_COND_MASK     0xf0000000
#define ARM_COND_SHIFT    28
#define ARM_OPCODE_MASK   0x01e00000
#define ARM_OPCODE_SHIFT  21
#define ARM_S_MASK        0x00100000
#define ARM_RN_MASK       0x000f0000
#define ARM_RN_SHIFT      16
#define ARM_RD_MASK       0x0000f000
#define ARM_RD_SHIFT      12
#define ARM_RM_MASK       0x0000000f
#define ARM_I_MASK        0x02000000
#define ARM_I_SHIFT       25

static const char *cond_codes[] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
};

static const char *reg_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
};

static const char *dp_opcodes[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
};

static const char *get_cond_suffix(uint32_t cond) {
    return cond_codes[cond & 0xf];
}

static const char *get_reg_name(uint32_t reg) {
    return reg_names[reg & 0xf];
}

static int32_t sign_extend_24(uint32_t val) {
    return ((int32_t)(val << 8)) >> 8;
}

instr_type_t decode_instruction_type(uint32_t instr) {
    uint32_t cond = (instr >> 28) & 0xf;
    uint32_t bits_27_26 = (instr >> 26) & 3;
    uint32_t bits_25_20 = (instr >> 20) & 0x3f;
    
    if (bits_27_26 == 0) {
        if ((bits_25_20 & 0x30) == 0x10 && (instr & 0x0ff00ff0) == 0x01200000) {
            return INSTR_TYPE_PSR_TRANSFER;
        }
        if ((bits_25_20 & 0x30) == 0x00 && (bits_25_20 & 0x0f) == 9) {
            return INSTR_TYPE_MULTIPLY;
        }
        if ((bits_25_20 & 0x30) == 0x10 && (bits_25_20 & 0x0f) == 9) {
            return INSTR_TYPE_MULTIPLY_LONG;
        }
        if ((bits_25_20 & 0x30) == 0x10 && (bits_25_20 & 0x0f) == 0) {
            if ((instr & 0x0f00) == 0) {
                return INSTR_TYPE_UNDEFINED;
            }
            if ((instr & 0x0000f000) == 0 && (instr & 0x0ff00ff0) == 0x01000090) {
                return INSTR_TYPE_SWAP;
            }
        }
        return INSTR_TYPE_DATA_PROC;
    } else if (bits_27_26 == 1) {
        return INSTR_TYPE_SINGLE_DATA_TRANSFER;
    } else if (bits_27_26 == 2) {
        if ((bits_25_20 & 0x20) == 0) {
            return INSTR_TYPE_BRANCH;
        }
        return INSTR_TYPE_BLOCK_DATA_TRANSFER;
    } else if (bits_27_26 == 3) {
        if ((bits_25_20 & 0x20) == 0) {
            if (cond == 0xf) {
                return INSTR_TYPE_SOFTWARE_INTERRUPT;
            }
            return INSTR_TYPE_COPROCESSOR;
        }
        if (cond != 0xf) {
            return INSTR_TYPE_COPROCESSOR;
        }
        return INSTR_TYPE_UNDEFINED;
    }
    
    return INSTR_TYPE_UNDEFINED;
}

static int disasm_data_proc(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t opcode = (raw & ARM_OPCODE_MASK) >> ARM_OPCODE_SHIFT;
    uint32_t s = (raw & ARM_S_MASK) >> 20;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t i = (raw & ARM_I_MASK) >> ARM_I_SHIFT;
    
    const char *opcode_str = dp_opcodes[opcode];
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rd_str = get_reg_name(rd);
    char op2_buf[64];
    operand_t op2;
    
    if (i) {
        op2 = operand_parse_imm(raw);
    } else {
        op2 = operand_parse_reg(raw);
    }
    operand_format(&op2, op2_buf, sizeof(op2_buf));
    
    int len;
    
    if (opcode == 8 || opcode == 9 || opcode == 10 || opcode == 11) {
        const char *rn_str = get_reg_name(rn);
        len = snprintf(output, outsize, "%s%s %s, %s", opcode_str, cond_str, rn_str, op2_buf);
    } else {
        if (opcode == 13 || opcode == 15) {
            len = snprintf(output, outsize, "%s%s %s, %s", opcode_str, cond_str, rd_str, op2_buf);
        } else {
            const char *rn_str = get_reg_name(rn);
            len = snprintf(output, outsize, "%s%s %s, %s, %s", opcode_str, cond_str, rd_str, rn_str, op2_buf);
        }
    }
    
    if (s && instr->cond != 0xe) {
        snprintf(output + len, outsize - len, "s");
    }
    
    return len;
}

static int disasm_multiply(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t a = (raw >> 21) & 1;
    uint32_t s = (raw >> 20) & 1;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rs = (raw >> 8) & 0xf;
    uint32_t rm = raw & 0xf;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rd_str = get_reg_name(rd);
    const char *rm_str = get_reg_name(rm);
    const char *rs_str = get_reg_name(rs);
    
    int len;
    
    if (a) {
        const char *rn_str = get_reg_name(rn);
        len = snprintf(output, outsize, "mla%s %s, %s, %s, %s", cond_str, rd_str, rm_str, rs_str, rn_str);
    } else {
        len = snprintf(output, outsize, "mul%s %s, %s, %s", cond_str, rd_str, rm_str, rs_str);
    }
    
    if (s && instr->cond != 0xe) {
        snprintf(output + len, outsize - len, "s");
    }
    
    return len;
}

static int disasm_single_data_transfer(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t i = (raw >> 25) & 1;
    uint32_t p = (raw >> 24) & 1;
    uint32_t u = (raw >> 23) & 1;
    uint32_t b = (raw >> 22) & 1;
    uint32_t w = (raw >> 21) & 1;
    uint32_t l = (raw >> 20) & 1;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t offset = raw & 0xfff;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rd_str = get_reg_name(rd);
    const char *rn_str = get_reg_name(rn);
    const char *op_str = l ? "ldr" : "str";
    const char *size_str = b ? "b" : "";
    
    char addr_buf[128];
    
    if (p) {
        if (i) {
            operand_t op = operand_parse_reg(raw);
            char offset_buf[64];
            operand_format(&op, offset_buf, sizeof(offset_buf));
            if (u) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, %s]%s", rn_str, offset_buf, w ? "!" : "");
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, -%s]%s", rn_str, offset_buf, w ? "!" : "");
            }
        } else {
            if (offset == 0) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s]%s", rn_str, w ? "!" : "");
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, #%s%u]%s", rn_str, u ? "" : "-", offset, w ? "!" : "");
            }
        }
    } else {
        if (i) {
            operand_t op = operand_parse_reg(raw);
            char offset_buf[64];
            operand_format(&op, offset_buf, sizeof(offset_buf));
            if (u) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], %s", rn_str, offset_buf);
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], -%s", rn_str, offset_buf);
            }
        } else {
            if (offset == 0) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s]", rn_str);
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], #%s%u", rn_str, u ? "" : "-", offset);
            }
        }
    }
    
    return snprintf(output, outsize, "%s%s%s %s, %s", op_str, size_str, cond_str, rd_str, addr_buf);
}

static int disasm_branch(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t l = (raw >> 24) & 1;
    int32_t offset = sign_extend_24(raw & 0x00ffffff) << 2;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *op_str = l ? "bl" : "b";
    
    return snprintf(output, outsize, "%s%s #%d", op_str, cond_str, offset);
}

static int disasm_software_interrupt(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t comment = raw & 0x00ffffff;
    
    return snprintf(output, outsize, "swi #%u", comment);
}

static int disasm_block_data_transfer(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t p = (raw >> 24) & 1;
    uint32_t u = (raw >> 23) & 1;
    uint32_t w = (raw >> 21) & 1;
    uint32_t l = (raw >> 20) & 1;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t reg_list = raw & 0xffff;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rn_str = get_reg_name(rn);
    const char *op_str = l ? "ldm" : "stm";
    const char *mode_str;
    
    if (p && u) mode_str = "ib";
    else if (p && !u) mode_str = "db";
    else if (!p && u) mode_str = "ia";
    else mode_str = "da";
    
    char reg_list_buf[256] = "";
    int first = 1;
    
    for (int i = 0; i < 16; i++) {
        if (reg_list & (1 << i)) {
            if (!first) {
                strcat(reg_list_buf, ", ");
            }
            strcat(reg_list_buf, get_reg_name(i));
            first = 0;
        }
    }
    
    return snprintf(output, outsize, "%s%s%s %s%s, {%s}", op_str, mode_str, cond_str, rn_str, w ? "!" : "", reg_list_buf);
}

int decode_instruction(const instruction_t *instr, char *output, size_t outsize) {
    switch (instr->type) {
        case INSTR_TYPE_DATA_PROC:
            return disasm_data_proc(instr, output, outsize);
            
        case INSTR_TYPE_MULTIPLY:
            return disasm_multiply(instr, output, outsize);
            
        case INSTR_TYPE_SINGLE_DATA_TRANSFER:
            return disasm_single_data_transfer(instr, output, outsize);
            
        case INSTR_TYPE_BRANCH:
            return disasm_branch(instr, output, outsize);
            
        case INSTR_TYPE_SOFTWARE_INTERRUPT:
            return disasm_software_interrupt(instr, output, outsize);
            
        case INSTR_TYPE_BLOCK_DATA_TRANSFER:
            return disasm_block_data_transfer(instr, output, outsize);
            
        case INSTR_TYPE_MULTIPLY_LONG:
        case INSTR_TYPE_SWAP:
        case INSTR_TYPE_PSR_TRANSFER:
        case INSTR_TYPE_HALFWORD_DATA_TRANSFER:
        case INSTR_TYPE_COPROCESSOR:
        case INSTR_TYPE_UNDEFINED:
        default:
            return snprintf(output, outsize, ".word 0x%08x", instr->raw);
    }
}

