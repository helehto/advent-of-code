#include "common.h"

namespace aoc_2016_6 {

static void solve(const std::vector<std::string_view> &lines,
                  std::array<std::array<uint8_t, 32>, 8> freqs,
                  const int step)
{
    for (std::string_view line : lines)
        for (size_t i = 0; i < line.size(); ++i)
            freqs[i][line[i] - 'a'] += step;

    for (size_t i = 0; i < 8; ++i) {
        auto it = std::ranges::min_element(freqs[i]);
        fputc('a' + std::distance(freqs[i].begin(), it), stdout);
    }

    fputc('\n', stdout);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::array<std::array<uint8_t, 32>, 8> freqs;

    memset(&freqs, 0xff, sizeof(freqs));
    solve(lines, freqs, -1);

    memset(&freqs, 0xff, sizeof(freqs));
    solve(lines, freqs, 1);
}
}
