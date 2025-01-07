#include "common.h"

namespace aoc_2019_1 {

void run(FILE *f)
{
    auto buf = slurp(f);
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
    fmt::print("{}\n{}\n", sum, sum2);
}

}
