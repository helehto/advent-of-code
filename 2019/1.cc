#include "common.h"

namespace aoc_2019_1 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto nums = find_numbers<int>(buf);

    int sum = 0;
    for (auto n : nums)
        sum += n / 3 - 2;

    int sum2 = 0;
    for (auto n : nums) {
        int s = 0;
        while (true) {
            n = n / 3 - 2;
            if (n <= 0)
                break;
            s += n;
        }
        sum2 += s;
    }
    answer.add(sum);
    answer.add(sum2);
}
AOC_REGISTER_SOLVER(2019, 1, run);

}
