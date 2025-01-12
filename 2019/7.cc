#include "common.h"

namespace aoc_2019_7 {

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

enum class HaltReason { op99, need_input };

struct IntcodeVM {
    std::vector<int> prog;
    std::vector<int> input;
    std::vector<int> output;
    size_t pc = 0;

    void reset(std::span<const int> prog)
    {
        this->prog.clear();
        this->prog.insert(end(this->prog), begin(prog), end(prog));
        input.clear();
        output.clear();
        pc = 0;
    }

    HaltReason run(std::initializer_list<int> extra_input = {});
};

inline HaltReason IntcodeVM::run(std::initializer_list<int> extra_input)
{
    input.insert(end(input), begin(extra_input), end(extra_input));

    auto operand = [&](int div, int offset) {
        const auto val = prog[pc + offset];
        return (prog[pc] / div) % 10 ? val : prog[val];
    };
    auto op1 = [&] { return operand(100, 1); };
    auto op2 = [&] { return operand(1000, 2); };

    while (true) {
        ASSERT(pc < prog.size());
        const int opcode = prog[pc] % 100;

        if (opcode == OP_HALT) {
            return HaltReason::op99;
        } else if (opcode == OP_ADD) {
            prog[prog[pc + 3]] = op1() + op2();
            pc += 4;
        } else if (opcode == OP_MUL) {
            prog[prog[pc + 3]] = op1() * op2();
            pc += 4;
        } else if (opcode == OP_IN) {
            if (input.empty())
                return HaltReason::need_input;
            prog[prog[pc + 1]] = input.front();
            input.erase(input.begin());
            pc += 2;
        } else if (opcode == OP_OUT) {
            output.push_back(op1());
            pc += 2;
        } else if (opcode == OP_JT) {
            pc = op1() ? op2() : pc + 3;
        } else if (opcode == OP_JF) {
            pc = !op1() ? op2() : pc + 3;
        } else if (opcode == OP_LT) {
            prog[prog[pc + 3]] = (op1() < op2());
            pc += 4;
        } else if (opcode == OP_EQ) {
            prog[prog[pc + 3]] = (op1() == op2());
            pc += 4;
        } else {
            ASSERT_MSG(false, "Unknown opcode {}", opcode);
        }
    }
}

static int part1(std::array<IntcodeVM, 5> &amplifiers, std::span<const int> prog)
{
    std::array<int8_t, 5> perm;
    for (size_t i = 0; i < perm.size(); ++i)
        perm[i] = i;

    int max_thruster_value = INT_MIN;
    do {
        int last_output = 0;
        for (size_t i = 0; i < 5; i++) {
            amplifiers[i].reset(prog);
            amplifiers[i].run({perm[i], last_output});
            last_output = amplifiers[i].output[0];
        }
        max_thruster_value = std::max(max_thruster_value, last_output);
    } while (std::next_permutation(begin(perm), end(perm)));

    return max_thruster_value;
}

static int part2(std::array<IntcodeVM, 5> &amplifiers, std::span<const int> prog)
{
    std::array<int8_t, 5> perm;
    for (size_t i = 0; i < perm.size(); ++i)
        perm[i] = i + 5;

    int max_thruster_value = INT_MIN;
    do {
        for (size_t i = 0; i < 5; i++) {
            amplifiers[i].reset(prog);
            amplifiers[i].input.push_back(perm[i]);
        }
        int last_thruster_value = 0;
        int last_output = 0;
        for (size_t i = 0; i < 5; i = (i < 4) ? i + 1 : 0) {
            HaltReason reason = amplifiers[i].run({last_output});
            ASSERT(amplifiers[i].output.size() == 1);
            last_output = amplifiers[i].output[0];
            amplifiers[i].output.clear();
            if (i == 4) {
                last_thruster_value = last_output;
                if (reason == HaltReason::op99)
                    break;
            }
        }
        max_thruster_value = std::max(max_thruster_value, last_thruster_value);
    } while (std::next_permutation(begin(perm), end(perm)));

    return max_thruster_value;
}

void run(FILE *f)
{
    auto buf = slurp(f);
    const auto prog = find_numbers<int>(buf);
    std::array<IntcodeVM, 5> amplifiers;
    fmt::print("{}\n", part1(amplifiers, prog));
    fmt::print("{}\n", part2(amplifiers, prog));
}

}
