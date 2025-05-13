#include "common.h"

namespace aoc_2015_23 {

constexpr uint32_t run_program(std::span<const std::string_view> lines,
                               std::array<uint32_t, 2> regs)
{
    for (size_t pc = 0; pc < lines.size();) {
        std::string_view line = lines[pc];

        if (line.starts_with("hlf")) {
            regs[line[4] - 'a'] /= 2;
            pc++;
        } else if (line.starts_with("tpl")) {
            regs[line[4] - 'a'] *= 3;
            pc++;
        } else if (line.starts_with("inc")) {
            regs[line[4] - 'a']++;
            pc++;
        } else if (line.starts_with("jmp")) {
            const auto [offset] = find_numbers_n<int, 1>(line);
            pc += offset;
        } else if (line.starts_with("jie")) {
            const auto [offset] = find_numbers_n<int, 1>(line);
            if (regs[line[4] - 'a'] % 2 == 0)
                pc += offset;
            else
                pc++;
        } else if (line.starts_with("jio")) {
            const auto [offset] = find_numbers_n<int, 1>(line);
            if (regs[line[4] - 'a'] == 1)
                pc += offset;
            else
                pc++;
        } else {
            ASSERT(false);
        }
    }

    return regs[1];
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    fmt::print("{}\n", run_program(lines, {0, 0}));
    fmt::print("{}\n", run_program(lines, {1, 0}));
}

}
