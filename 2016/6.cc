#include <algorithm>
#include <aoc/base.h>
#include <aoc/string.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace aoc_2016_6 {

static std::string solve(const std::vector<std::string_view> &lines,
                         std::array<std::array<uint8_t, 32>, 8> freqs,
                         const int step)
{
    for (std::string_view line : lines)
        for (size_t i = 0; i < line.size(); ++i)
            freqs[i][line[i] - 'a'] += step;

    std::string result;
    for (size_t i = 0; i < 8; ++i) {
        auto it = std::ranges::min_element(freqs[i]);
        result += 'a' + std::distance(freqs[i].begin(), it);
    }

    return result;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);

    std::array<std::array<uint8_t, 32>, 8> freqs;

    memset(&freqs, 0xff, sizeof(freqs));
    answer.add(solve(lines, freqs, -1));

    memset(&freqs, 0xff, sizeof(freqs));
    answer.add(solve(lines, freqs, 1));
}
AOC_REGISTER_SOLVER(2016, 6, run);

}
