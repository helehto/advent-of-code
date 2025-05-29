#include "common.h"
#include "vm.h"

namespace aoc_2018_16 {

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
