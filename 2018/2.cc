#include <algorithm>
#include <aoc/base.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace aoc_2018_2 {

void run(std::string_view buf, aoc::Answer &answer)
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
    answer.add(twos * threes);

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
                answer.add_formatted("{}{}", a.substr(0, index), a.substr(index + 1));
                return;
            }
        }
    }
}
AOC_REGISTER_SOLVER(2018, 2, run);

}
