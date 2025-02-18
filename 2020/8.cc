#include "common.h"

namespace aoc_2020_8 {

enum { NOP, JMP, ACC };
struct Instruction {
    uint16_t op : 2;
    int16_t val : 14;
};

std::pair<int, bool> run(const std::vector<Instruction> &prog)
{
    std::vector<bool> executed(prog.size(), false);
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

    std::vector<Instruction> prog;
    std::vector<int> nums;
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

        find_numbers(line, nums);
        ASSERT(nums.size() == 1);
        instr.val = nums[0];
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
