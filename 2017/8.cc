#include "common.h"
#include "dense_map.h"

namespace aoc_2017_8 {

void run(std::string_view buf)
{
    std::vector<std::string_view> toks;
    auto lines = split_lines(buf);
    dense_map<std::string_view, int> regs;
    regs.reserve(lines.size());

    int peak_value = INT_MIN;
    for (std::string_view line : lines) {
        split(line, toks, ' ');

        const auto [rhs] = find_numbers_n<int, 1>(toks[6]);
        auto cmp = toks[5];
        int cmp_value = regs[toks[4]];
        if ((cmp == ">" && !(cmp_value > rhs)) || (cmp == ">=" && !(cmp_value >= rhs)) ||
            (cmp == "<" && !(cmp_value < rhs)) || (cmp == "<=" && !(cmp_value <= rhs)) ||
            (cmp == "==" && !(cmp_value == rhs)) || (cmp == "!=" && !(cmp_value != rhs)))
            continue;

        auto &value = regs[toks[0]];
        const auto [n] = find_numbers_n<int, 1>(toks[2]);
        value += toks[1] == "inc" ? n : -n;
        peak_value = std::max(peak_value, value);
    }

    using namespace std::ranges;
    fmt::print("{}\n", *max_element(views::values(regs)));
    fmt::print("{}\n", peak_value);
}
}
