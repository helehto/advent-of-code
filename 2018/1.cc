#include "common.h"
#include "dense_set.h"
#include <numeric>

namespace aoc_2018_1 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto nums = find_numbers<int>(buf);
    fmt::print("{}\n", std::accumulate(nums.begin(), nums.end(), 0));

    dense_set<int> seen;
    seen.reserve(100'000);
    int freq = 0;
    while (true) {
        for (int x : nums) {
            if (!seen.insert(freq).second) {
                fmt::print("{}\n", freq);
                return;
            }
            freq += x;
        }
    }
}

}
