#include "common.h"

namespace aoc_2020_8 {

enum { NOP, JMP, ACC };
struct Instruction {
    uint16_t op : 2;
    int16_t val : 14;
};

std::pair<int, bool> run(std::span<const Instruction> prog)
{
    small_vector<bool, 1024> executed(prog.size(), false);
    int acc = 0;
    size_t pc = 0;
    while (pc < prog.size() && !executed[pc]) {
        executed[pc] = true;
        int op = prog[pc].op;
        int val = prog[pc].val;
        if (op == ACC) {
            acc += val;
            pc++;
        }
        if (op == JMP) {
            pc += val;
        }
        if (op == NOP) {
            pc++;
        }
    }

    return {acc, pc >= prog.size()};
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    small_vector<Instruction, 1024> prog;
    prog.reserve(size(lines));
    for (std::string_view line : lines) {
        Instruction &instr = prog.emplace_back();
        std::string_view mnemonic = line.substr(0, 3);
        if (mnemonic == "nop")
            instr.op = NOP;
        else if (mnemonic == "acc")
            instr.op = ACC;
        else if (mnemonic == "jmp")
            instr.op = JMP;
        else
            ASSERT(false);

        instr.val = find_numbers_n<int, 1>(line)[0];
    }

    // Part 1:
    fmt::print("{}\n", run(prog).first);

    // Part 2:
    for (size_t i = 0; i < prog.size(); i++) {
        prog[i].op ^= 1;
        if (auto [acc, terminated] = run(prog); terminated) {
            fmt::print("{}\n", acc);
            break;
        }
        prog[i].op ^= 1;
    }
}

}
