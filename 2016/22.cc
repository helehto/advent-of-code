#include "common.h"

namespace aoc_2016_22 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    int viable_pairs = 0;
    for (size_t i = 2; i < lines.size(); ++i) {
        const auto [x_a, y_a, size_a, used_a, avail_a, usepct_a] =
            find_numbers_n<int, 6>(lines[i]);

        for (size_t j = 2; j < lines.size(); ++j) {
            if (i == j)
                continue;

            const auto [x_b, y_b, size_b, used_b, avail_b, usepct_b] =
                find_numbers_n<int, 6>(lines[j]);

            if (used_a != 0 && used_a <= avail_b)
                viable_pairs++;
        }
    }

    fmt::print("{}\n", viable_pairs);
}

}
