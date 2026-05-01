#pragma once

#include "common.h"

enum Opcode : int64_t {
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
    OP_HALT,
};
constexpr size_t NUM_OPCODES = 13;

struct Instruction {
    void *handler;
    Opcode opcode;
    int64_t op1;
    int64_t op2;
};
static_assert(sizeof(Instruction) == 32);

struct Program {
    std::vector<Instruction> instructions;
};

inline Program assemble(std::string_view buf)
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
                result.push_back({nullptr, OP_CPY_IR, imm_op(1), reg_op(2)});
            } else {
                result.push_back({nullptr, OP_CPY_RR, reg_op(1), reg_op(2)});
            }
        } else if (tokens[0] == "inc") {
            result.push_back({nullptr, OP_INC_R, reg_op(1)});
        } else if (tokens[0] == "dec") {
            result.push_back({nullptr, OP_DEC_R, reg_op(1)});
        } else if (tokens[0] == "jnz") {
            if (int_p(1)) {
                if (int_p(2)) {
                    result.push_back({nullptr, OP_JNZ_II, imm_op(1), imm_op(2)});
                } else {
                    result.push_back({nullptr, OP_JNZ_IR, imm_op(1), reg_op(2)});
                }
            } else {
                if (int_p(2)) {
                    result.push_back({nullptr, OP_JNZ_RI, reg_op(1), imm_op(2)});
                } else {
                    result.push_back({nullptr, OP_JNZ_RR, reg_op(1), reg_op(2)});
                }
            }
        } else if (tokens[0] == "tgl") {
            ASSERT(!int_p(1));
            result.push_back({nullptr, OP_TGL_R, reg_op(1)});
        } else {
            ASSERT(false);
        }
    }

    result.push_back({nullptr, OP_HALT});
    return {std::move(result)};
}

constexpr static auto toggle_opcode_table = [] consteval {
    std::array<int64_t, NUM_OPCODES> tab;
    tab.fill(-1);
    tab[OP_JNZ_II] = OP_NOP;
    tab[OP_JNZ_RI] = OP_NOP;
    tab[OP_INC_R] = OP_DEC_R;
    tab[OP_DEC_R] = OP_INC_R;
    tab[OP_TGL_R] = OP_INC_R;
    tab[OP_CPY_IR] = OP_JNZ_IR;
    tab[OP_CPY_RR] = OP_JNZ_RR;
    tab[OP_JNZ_IR] = OP_CPY_IR;
    tab[OP_JNZ_RR] = OP_CPY_RR;
    tab[OP_HALT] = OP_HALT;
    return tab;
}();

static void optimize(Program &p)
{
    auto &prog = p.instructions;

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

            if (cpy1->opcode == OP_CPY_RR && cpy2->opcode == OP_CPY_RR &&
                add->opcode == OP_ADD_RR && dec->opcode == OP_DEC_R &&
                jnz->opcode == OP_JNZ_RI) {
                auto outer_factor_reg = cpy1->op2;
                auto inner_factor_reg = cpy2->op1;
                auto result_reg = add->op2;

                add[0] = {nullptr, OP_MUL_RR, inner_factor_reg, outer_factor_reg};
                add[2] = {nullptr, OP_CPY_RR, outer_factor_reg, result_reg};
                add[3] = {nullptr, OP_CPY_IR, 0, outer_factor_reg};
                add[4] = {nullptr, OP_NOP, 0, 0};

                changed = true;
            }
        }
    } while (changed);
}

inline int run_program(Program &prog, std::array<int64_t, 4> regs)
{
    static void *const dispatch_table[] = {
        &&nop,    &&cpy_ir, &&cpy_rr, &&inc_r,  &&dec_r, &&jnz_ii, &&jnz_ri,
        &&jnz_ir, &&jnz_rr, &&add_rr, &&mul_rr, &&tgl_r, &&halt,
    };
    std::span<Instruction> instrs(prog.instructions);

    // Store the handler for each instruction upfront to avoid an extra memory
    // load from `dispatch_table` for each instruction executed.
    for (Instruction &i : instrs)
        i.handler = dispatch_table[i.opcode];

    // -1 as the first iteration starts on `nop` for the first DISPATCH().
    Instruction *inst = instrs.data() - 1;
    const Instruction *const end = instrs.data() + instrs.size();

#define DISPATCH()                                                                       \
    do {                                                                                 \
        inst++;                                                                          \
        DEBUG_ASSERT(inst >= instrs.data() && inst < end);                               \
        /* This is never out of bounds; the last instruction is always HALT .*/          \
        goto * inst->handler;                                                            \
    } while (0)

    while (true) {
    nop:
        DISPATCH();
    cpy_ir:
        regs[inst->op2] = inst->op1;
        DISPATCH();
    cpy_rr:
        regs[inst->op2] = regs[inst->op1];
        DISPATCH();
    inc_r:
        regs[inst->op1]++;
        DISPATCH();
    dec_r:
        regs[inst->op1]--;
        DISPATCH();
    jnz_ii:
        if (inst->op1) {
            inst += inst->op2 - 1;
            if (inst >= end) [[unlikely]]
                goto halt;
        }
        DISPATCH();
    jnz_ri:
        if (regs[inst->op1]) {
            inst += inst->op2 - 1;
            if (inst >= end) [[unlikely]]
                goto halt;
        }
        DISPATCH();
    jnz_ir:
        if (inst->op1) {
            inst += regs[inst->op2] - 1;
            if (inst >= end) [[unlikely]]
                goto halt;
        }
        DISPATCH();
    jnz_rr:
        if (regs[inst->op1]) {
            inst += regs[inst->op2] - 1;
            if (inst >= end) [[unlikely]]
                goto halt;
        }
        DISPATCH();
    add_rr:
        regs[inst->op2] += regs[inst->op1];
        DISPATCH();
    mul_rr:
        regs[inst->op2] *= regs[inst->op1];
        DISPATCH();
    tgl_r:
        if (int64_t offset = regs[inst->op1];
            inst >= instrs.data() && inst + offset < end) {
            Instruction &target = inst[offset];
            const int64_t new_opcode = toggle_opcode_table[target.opcode];
            ASSERT(new_opcode != -1);
            target.opcode = static_cast<Opcode>(new_opcode);
            target.handler = dispatch_table[target.opcode];
        }
        DISPATCH();
    halt:
        return regs[0];
    }

#undef DISPATCH
}
