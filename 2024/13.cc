#include "common.h"

namespace aoc_2024_13 {

void run(FILE *f)
{
    auto buf = slurp(f);
    std::vector<int> nums;
    find_numbers(buf, nums);

    int64_t s[2]{};
    for (size_t i = 0; i < nums.size(); i += 6) {
        const int64_t a = nums[i + 0];
        const int64_t b = nums[i + 1];
        const int64_t c = nums[i + 2];
        const int64_t d = nums[i + 3];
        const int64_t u = nums[i + 4];
        const int64_t v = nums[i + 5];

        const int64_t det = a * d - b * c;

        for (int j = 0; j < 2; ++j) {
            const int64_t N = INT64_C(10000000000000) * j;
            int64_t x = (d * (u + N) - c * (v + N)) / det;
            int64_t y = (a * (v + N) - b * (u + N)) / det;
            if (a * x + c * y == u + N && b * x + d * y == v + N)
                s[j++] += 3 * x + y;
        }
    }

    fmt::print("{}\n", s[0]);
    fmt::print("{}\n", s[1]);
}

}
