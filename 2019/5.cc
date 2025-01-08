#include "common.h"

namespace aoc_2019_5 {

enum {
    OP_ADD = 1,
    OP_MUL = 2,
    OP_IN = 3,
    OP_OUT = 4,
    OP_JT = 5,
    OP_JF = 6,
    OP_LT = 7,
    OP_EQ = 8,
    OP_HALT = 99,
};

inline std::vector<int> run_program(std::vector<int> &prog, const std::vector<int> &input)
{
    std::vector<int> output;
    size_t pc = 0;
    size_t input_index = 0;

    auto operand = [&](int div, int offset) {
        const auto val = prog[pc + offset];
        return (prog[pc] / div) % 10 ? val : prog[val];
    };
    auto op1 = [&] { return operand(100, 1); };
    auto op2 = [&] { return operand(1000, 2); };

    while (true) {
        ASSERT(pc < prog.size());
        const int opcode = prog[pc] % 100;
        const int imm3 = pc + 3 < prog.size() ? prog[pc + 3] : -999999;

        if (opcode == OP_HALT) {
            return output;
        } else if (opcode == OP_ADD) {
            prog[imm3] = op1() + op2();
            pc += 4;
        } else if (opcode == OP_MUL) {
            prog[imm3] = op1() * op2();
            pc += 4;
        } else if (opcode == OP_IN) {
            prog[prog[pc + 1]] = input[input_index++];
            pc += 2;
        } else if (opcode == OP_OUT) {
            output.push_back(op1());
            pc += 2;
        } else if (opcode == OP_JT) {
            pc = op1() ? op2() : pc + 3;
        } else if (opcode == OP_JF) {
            pc = !op1() ? op2() : pc + 3;
        } else if (opcode == OP_LT) {
            prog[imm3] = (op1() < op2());
            pc += 4;
        } else if (opcode == OP_EQ) {
            prog[imm3] = (op1() == op2());
            pc += 4;
        } else {
            ASSERT_MSG(false, "Unknown opcode {}", opcode);
        }
    }
}

void run(FILE *f)
{
    auto buf = slurp(f);
    auto orig_prog = find_numbers<int>(buf);
    auto prog = orig_prog;
    fmt::print("{}\n", run_program(prog, {1}).back());
    prog = orig_prog;
    fmt::print("{}\n", run_program(prog, {5}).back());
}

}
