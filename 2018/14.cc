#include "common.h"

namespace aoc_2018_14 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto n = find_numbers<size_t>(lines[0])[0];

    std::vector<uint8_t> scores;
    scores.reserve(30'000'000);
    scores.push_back(3);
    scores.push_back(7);

    size_t i = 0;
    size_t j = 1;

    auto step = [&] {
        int s = scores[i] + scores[j];
        if (s >= 10)
            scores.push_back(s / 10);
        scores.push_back(s % 10);
        i += 1 + scores[i];
        j += 1 + scores[j];
        while (i >= scores.size())
            i -= scores.size();
        while (j >= scores.size())
            j -= scores.size();
    };

    while (n + 10 >= scores.size())
        step();

    for (size_t k = n + 0; k < n + 10; k++)
        fmt::print("{}", scores[k]);
    fmt::print("\n");

    const size_t needle_size = lines[0].size();
    uint64_t needle = 0;
    for (char c : lines[0])
        needle = (needle << 8) | (c - '0');

    uint64_t window = 0;
    const uint64_t mask = (UINT64_C(1) << (8 * lines[0].size())) - 1;
    for (size_t i = scores.size() - needle_size; i < scores.size(); i++)
        window = (window << 8) | scores[i];

    size_t m = scores.size() - needle_size;
    while (true) {
        while (m + needle_size < scores.size()) {
            if (needle == (window & mask)) {
                fmt::print("{}\n", m);
                return;
            }
            window = (window << 8) | scores[m + needle_size];
            m++;
        }
        step();
    }
}

}
