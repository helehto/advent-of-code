#include "common.h"
#include <algorithm>
#include <vector>

namespace aoc_2018_16 {

enum Instruction {
    instr_addr,
    instr_addi,
    instr_mulr,
    instr_muli,
    instr_banr,
    instr_bani,
    instr_borr,
    instr_bori,
    instr_setr,
    instr_seti,
    instr_gtir,
    instr_gtri,
    instr_gtrr,
    instr_eqir,
    instr_eqri,
    instr_eqrr,
    num_opcodes,
};

static void execute(std::array<int, 4> &regs, const std::array<int, 4> &instr)
{
    auto rr = [&](auto &&f) { regs[instr[3]] = f(regs[instr[1]], regs[instr[2]]); };
    auto ri = [&](auto &&f) { regs[instr[3]] = f(regs[instr[1]], instr[2]); };
    auto ir = [&](auto &&f) { regs[instr[3]] = f(instr[1], regs[instr[2]]); };

    switch (instr[0]) {
    case instr_addr:
        rr(std::plus<>());
        break;
    case instr_addi:
        ri(std::plus<>());
        break;
    case instr_mulr:
        rr(std::multiplies<>());
        break;
    case instr_muli:
        ri(std::multiplies<>());
        break;
    case instr_banr:
        rr(std::bit_and<>());
        break;
    case instr_bani:
        ri(std::bit_and<>());
        break;
    case instr_borr:
        rr(std::bit_or<>());
        break;
    case instr_bori:
        ri(std::bit_or<>());
        break;
    case instr_setr:
        rr(λab(a));
        break;
    case instr_seti:
        ir(λab(a));
        break;
    case instr_gtir:
        ir(std::greater<>());
        break;
    case instr_gtri:
        ri(std::greater<>());
        break;
    case instr_gtrr:
        rr(std::greater<>());
        break;
    case instr_eqir:
        ir(std::equal_to<>());
        break;
    case instr_eqri:
        ri(std::equal_to<>());
        break;
    case instr_eqrr:
        rr(std::equal_to<>());
        break;
    }
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::array<int, 4> input_regs;
    std::array<int, 4> expected_regs;
    std::array<int, 4> instr;
    std::array<int, 4> candidate;
    std::array<int, 4> output_regs;

    uint32_t masks[num_opcodes];
    std::ranges::fill(masks, (UINT32_C(1) << num_opcodes) - 1);

    int part1 = 0;
    size_t i = 0;
    for (; lines[i].starts_with("Before:"); i += 4) {
        input_regs = find_numbers_n<int, 4>(lines[i]);
        instr = find_numbers_n<int, 4>(lines[i + 1]);
        expected_regs = find_numbers_n<int, 4>(lines[i + 2]);

        int matches = 0;
        for (int op = 0; op < num_opcodes; op++) {
            candidate = instr;
            candidate[0] = op;
            output_regs = input_regs;
            execute(output_regs, candidate);
            if (expected_regs == output_regs)
                matches++;
            else
                masks[instr[0]] &= ~(UINT32_C(1) << op);
        }

        part1 += matches >= 3;
    }
    fmt::print("{}\n", part1);

restart:
    for (size_t j = 0; j < std::size(masks); j++) {
        if ((masks[j] & (masks[j] - 1)) == 0) {
            for (size_t k = 0; k < std::size(masks); k++) {
                if (k != j && (masks[k] & masks[j]) != 0) {
                    masks[k] &= ~masks[j];
                    goto restart;
                }
            }
        }
    }

    int permutation[num_opcodes];
    for (size_t j = 0; j < std::size(masks); ++j)
        permutation[j] = std::countr_zero(masks[j]);

    while (lines[i].empty())
        i++;
    std::array<int, 4> regs{0, 0, 0, 0};
    for (; i < lines.size(); i++) {
        instr = find_numbers_n<int, 4>(lines[i]);
        instr[0] = permutation[instr[0]];
        execute(regs, instr);
    }

    fmt::print("{}\n", regs[0]);
}

}
