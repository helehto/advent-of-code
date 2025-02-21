#include "common.h"

namespace aoc_2016_12 {

enum Opcode {
    OP_NOP,
    OP_CPY_IR,
    OP_CPY_RR,
    OP_INC,
    OP_DEC,
    OP_JNZ_II,
    OP_JNZ_RI,
    OP_ADD,
};

struct Instruction {
    Opcode opcode;
    int32_t op1;
    int32_t op2;
};

static std::vector<Instruction> assemble(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<Instruction> result;
    result.reserve(buf.size());

    std::vector<std::string_view> tokens;
    for (std::string_view line : lines) {
        split(line, tokens, ' ');

        auto int_p = [&](int i) { return tokens[i][0] >= '0' && tokens[i][0] <= '9'; };
        auto imm_op = [&](int i) { return find_numbers_n<int, 1>(tokens[i])[0]; };
        auto reg_op = [&](int i) { return tokens[i][0] - 'a'; };

        if (tokens[0] == "cpy") {
            if (int_p(1)) {
                result.push_back({OP_CPY_IR, imm_op(1), reg_op(2)});
            } else {
                result.push_back({OP_CPY_RR, reg_op(1), reg_op(2)});
            }
        } else if (tokens[0] == "inc") {
            result.push_back({OP_INC, reg_op(1)});
        } else if (tokens[0] == "dec") {
            result.push_back({OP_DEC, reg_op(1)});
        } else if (tokens[0] == "jnz") {
            if (int_p(1)) {
                result.push_back({OP_JNZ_II, imm_op(1), imm_op(2)});
            } else {
                result.push_back({OP_JNZ_RI, reg_op(1), imm_op(2)});
            }
        } else {
            ASSERT(false);
        }
    }

    return result;
}

static void optimize(std::vector<Instruction> &prog)
{
    // Peephole optimization: detect addition implemented using repeated pairs
    // of `inc` and `dec`. The vast majority of the running time in a naively
    // interpreted version of the original program spends virtually 100% of its
    // time inside one of these loops.
    for (size_t i = 0; i + 2 < prog.size(); i++) {
        Instruction &inc = prog[i];
        Instruction &dec = prog[i + 1];
        Instruction &jnz = prog[i + 2];

        if (inc.opcode == OP_INC && dec.opcode == OP_DEC && jnz.opcode == OP_JNZ_RI &&
            jnz.op1 == dec.op1) {
            // Perform the addition in one go:
            inc.opcode = OP_ADD;
            inc.op2 = dec.op1;

            // Zero the induction variable:
            dec.opcode = OP_CPY_IR;
            dec.op2 = dec.op1;
            dec.op1 = 0;

            // Replace the jump with a NOP to preserve the length of the
            // instruction sequence; that way we don't have to patch jumps that
            // cross it.
            jnz.opcode = {};
        }
    }
}

static int run_program(const std::vector<Instruction> &prog, int cinit)
{
    std::array<int, 4> regs{0, 0, cinit, 0};

    for (size_t i = 0; i < prog.size(); ++i) {
        auto [opcode, op1, op2] = prog[i];
        if (opcode == OP_CPY_IR) {
            regs[op2] = op1;
        } else if (opcode == OP_CPY_RR) {
            regs[op2] = regs[op1];
        } else if (opcode == OP_INC) {
            regs[op1]++;
        } else if (opcode == OP_DEC) {
            regs[op1]--;
        } else if (opcode == OP_JNZ_II) {
            if (op1)
                i += op2 - 1;
        } else if (opcode == OP_JNZ_RI) {
            if (regs[op1])
                i += op2 - 1;
        } else if (opcode == OP_ADD) {
            regs[op1] += regs[op2];
        }
    }

    return regs[0];
}

void run(std::string_view buf)
{
    auto prog = assemble(buf);
    optimize(prog);
    fmt::print("{}\n", run_program(prog, 0));
    fmt::print("{}\n", run_program(prog, 1));
}

}
