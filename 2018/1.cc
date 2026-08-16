#include "common.h"
#include "dense_set.h"
#include <numeric>

namespace aoc_2018_1 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto nums = find_numbers<int>(buf);
    answer.add(std::accumulate(nums.begin(), nums.end(), 0));

    dense_set<int> seen;
    seen.reserve(100'000);
    int freq = 0;
    while (true) {
        for (int x : nums) {
            if (!seen.insert(freq).second) {
                answer.add(freq);
                return;
            }
            freq += x;
        }
    }
}
AOC_REGISTER_SOLVER(2018, 1, run);

}
