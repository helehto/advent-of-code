#pragma once

#include "common.h"

enum Opcode {
    OP_NOP,
    OP_CPY_IR,
    OP_CPY_RR,
    OP_INC_R,
    OP_DEC_R,
    OP_JNZ_II,
    OP_JNZ_RI,
    OP_JNZ_IR,
    OP_JNZ_RR,
    OP_ADD_RR,
    OP_MUL_RR,
    OP_TGL_R,
};

struct Instruction {
    Opcode opcode;
    int32_t op1;
    int32_t op2;
};

inline std::vector<Instruction> assemble(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<Instruction> result;
    result.reserve(buf.size());

    std::vector<std::string_view> tokens;
    for (std::string_view line : lines) {
        split(line, tokens, ' ');

        auto int_p = [&](int i) {
            return (tokens[i][0] >= '0' && tokens[i][0] <= '9') || tokens[i][0] == '-';
        };
        auto imm_op = [&](int i) { return find_numbers_n<int, 1>(tokens[i])[0]; };
        auto reg_op = [&](int i) { return tokens[i][0] - 'a'; };

        if (tokens[0] == "cpy") {
            if (int_p(1)) {
                result.push_back({OP_CPY_IR, imm_op(1), reg_op(2)});
            } else {
                result.push_back({OP_CPY_RR, reg_op(1), reg_op(2)});
            }
        } else if (tokens[0] == "inc") {
            result.push_back({OP_INC_R, reg_op(1)});
        } else if (tokens[0] == "dec") {
            result.push_back({OP_DEC_R, reg_op(1)});
        } else if (tokens[0] == "jnz") {
            if (int_p(1)) {
                if (int_p(2)) {
                    result.push_back({OP_JNZ_II, imm_op(1), imm_op(2)});
                } else {
                    result.push_back({OP_JNZ_IR, imm_op(1), reg_op(2)});
                }
            } else {
                if (int_p(2)) {
                    result.push_back({OP_JNZ_RI, reg_op(1), imm_op(2)});
                } else {
                    result.push_back({OP_JNZ_RR, reg_op(1), reg_op(2)});
                }
            }
        } else if (tokens[0] == "tgl") {
            ASSERT(!int_p(1));
            result.push_back({OP_TGL_R, reg_op(1)});
        } else {
            ASSERT(false);
        }
    }

    return result;
}

inline void toggle_instruction(Instruction &instr)
{
    switch (instr.opcode) {
    case OP_NOP:
    case OP_ADD_RR:
    case OP_MUL_RR:
        ASSERT(false);
        break;

    case OP_JNZ_II:
    case OP_JNZ_RI:
        instr.opcode = OP_NOP;
        break;

    case OP_INC_R:
        instr.opcode = OP_DEC_R;
        break;
    case OP_TGL_R:
    case OP_DEC_R:
        instr.opcode = OP_INC_R;
        break;

    case OP_CPY_IR:
        instr.opcode = OP_JNZ_IR;
        break;
    case OP_CPY_RR:
        instr.opcode = OP_JNZ_RR;
        break;

    case OP_JNZ_IR:
        instr.opcode = OP_CPY_IR;
        break;
    case OP_JNZ_RR:
        instr.opcode = OP_CPY_RR;
        break;
    }
}

static void optimize(std::vector<Instruction> &prog)
{
    // Peephole optimization:
    bool changed;
    do {
        changed = false;

        // Detect addition implemented using repeated pairs of `inc` and `dec`.
        // The vast majority of the running time in 2024/12 in a naively
        // interpreted version of the original program spends virtually 100% of
        // its time inside one of these loops.
        for (size_t i = 0; i + 2 < prog.size(); i++) {
            Instruction &inc = prog[i];
            Instruction &dec = prog[i + 1];
            Instruction &jnz = prog[i + 2];

            if (inc.opcode == OP_INC_R && dec.opcode == OP_DEC_R &&
                jnz.opcode == OP_JNZ_RI && jnz.op1 == dec.op1) {
                // Perform the addition in one go:
                inc.opcode = OP_ADD_RR;
                inc.op2 = inc.op1;
                inc.op1 = dec.op1;

                // Zero the induction variable:
                dec.opcode = OP_CPY_IR;
                dec.op2 = dec.op1;
                dec.op1 = 0;

                // Replace the jump with a NOP to preserve the length of the
                // instruction sequence; that way we don't have to patch jumps that
                // cross it.
                jnz.opcode = {};

                changed = true;
            }
        }

        // Detect multiplication implemented using repeated pairs of `add` and
        // `dec` + `jnz`. The majority of the time in 2024/23 is spent doing
        // this (even with the inc/dec -> add optimization taken into account).
        for (size_t i = 3; i + 4 < prog.size(); i++) {
            if (prog[i].opcode != OP_ADD_RR)
                continue;

            Instruction *cpy1 = &prog[i - 3];
            Instruction *cpy2 = &prog[i - 1];
            Instruction *add = &prog[i];
            Instruction *dec = &prog[i + 3];
            Instruction *jnz = &prog[i + 4];

            if (cpy1->opcode == OP_CPY_RR && cpy1->opcode == OP_CPY_RR &&
                add->opcode == OP_ADD_RR && dec->opcode == OP_DEC_R &&
                jnz->opcode == OP_JNZ_RI) {
                auto outer_factor_reg = cpy1->op2;
                auto inner_factor_reg = cpy2->op1;
                auto result_reg = add->op2;

                add[0] = {OP_MUL_RR, inner_factor_reg, outer_factor_reg};
                add[2] = {OP_CPY_RR, outer_factor_reg, result_reg};
                add[3] = {OP_CPY_IR, 0, outer_factor_reg};
                add[4] = {OP_NOP, 0, 0};

                changed = true;
            }
        }
    } while (changed);
}

inline int run_program(std::vector<Instruction> &prog, std::array<int, 4> regs)
{
    for (size_t i = 0; i < prog.size(); ++i) {
        auto [opcode, op1, op2] = prog[i];
        if (opcode == OP_CPY_IR) {
            regs[op2] = op1;
        } else if (opcode == OP_CPY_RR) {
            regs[op2] = regs[op1];
        } else if (opcode == OP_INC_R) {
            regs[op1]++;
        } else if (opcode == OP_DEC_R) {
            regs[op1]--;
        } else if (opcode == OP_JNZ_II) {
            if (op1)
                i += op2 - 1;
        } else if (opcode == OP_JNZ_RI) {
            if (regs[op1])
                i += op2 - 1;
        } else if (opcode == OP_JNZ_IR) {
            if (op1)
                i += regs[op2] - 1;
        } else if (opcode == OP_JNZ_RR) {
            if (regs[op1])
                i += regs[op2] - 1;
        } else if (opcode == OP_ADD_RR) {
            regs[op2] += regs[op1];
        } else if (opcode == OP_MUL_RR) {
            regs[op2] *= regs[op1];
        } else if (opcode == OP_TGL_R) {
            if (i + regs[op1] < prog.size())
                toggle_instruction(prog[i + regs[op1]]);
        }
    }

    return regs[0];
}
