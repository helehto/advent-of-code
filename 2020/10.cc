#include "common.h"
#include "dense_map.h"

namespace aoc_2020_10 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<int> joltages;
    joltages.reserve(lines.size());
    find_numbers(buf, joltages);

    joltages.push_back(0);
    std::sort(begin(joltages), end(joltages));
    joltages.push_back(joltages.back() + 3);

    int v1 = 0;
    int v3 = 0;
    for (size_t i = 1; i < joltages.size(); i++) {
        v1 += (joltages[i] - joltages[i - 1] == 1);
        v3 += (joltages[i] - joltages[i - 1] == 3);
    }
    fmt::print("{}\n", v1 * v3);

    dense_map<int, uint64_t> combinations{{joltages.back(), 1}};
    combinations.reserve(4 * lines.size());
    for (size_t i = joltages.size() - 1; i--;) {
        const size_t n = joltages[i];
        combinations[n] = combinations[n + 1] + combinations[n + 2] + combinations[n + 3];
    }
    fmt::print("{}\n", combinations[0]);
}

}
