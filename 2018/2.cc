#include "common.h"
#include <algorithm>

namespace aoc_2018_2 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    int twos = 0;
    int threes = 0;
    for (auto line : lines) {
        int counts[26] = {0};
        for (uint8_t c : line)
            counts[c - 'a']++;

        if (std::ranges::count(counts, 2) != 0)
            twos++;
        if (std::ranges::count(counts, 3) != 0)
            threes++;
    }
    fmt::print("{}\n", twos * threes);

    for (size_t i = 0; i < lines.size(); i++) {
        for (size_t j = i + 1; j < lines.size(); j++) {
            auto a = lines[i];
            auto b = lines[j];

            int diffs = 0;
            size_t index = 0;
            for (size_t k = 0; k < a.size() && diffs <= 1; k++) {
                if (a[k] != b[k]) {
                    index = k;
                    diffs++;
                }
            }

            if (diffs == 1) {
                fmt::print("{}{}\n", a.substr(0, index), a.substr(index + 1));
                return;
            }
        }
    }
}

}
