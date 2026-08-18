#include <aoc/base.h>
#include <aoc/dense_map.h>
#include <aoc/hash.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <cstdint>
#include <string_view>

namespace aoc_2024_11 {

[[gnu::noinline]]
static void step(dense_map<int64_t, int64_t, CrcHasher> &counter,
                 dense_map<int64_t, int64_t, CrcHasher> &result)
{
    result.clear();

    for (auto [num, count] : counter) {
        if (num == 0) [[unlikely]] {
            result[1] += count;
        } else if (const auto d = digit_count_base10(num); d % 2 == 0) {
            const auto p = pow10i[d / 2];
            result[num / p] += count;
            result[num % p] += count;
        } else {
            result[num * 2024] += count;
        }
    }
}

void run(std::string_view buf, aoc::Answer &answer)
{
    dense_map<int64_t, int64_t, CrcHasher> counter;
    dense_map<int64_t, int64_t, CrcHasher> tmp;
    counter.reserve(4096);
    tmp.reserve(4096);

    for (auto n : find_numbers_small<int64_t>(buf))
        counter[n]++;

    for (int steps : {25, 50}) {
        for (int64_t i = 0; i < steps; i++) {
            step(counter, tmp);
            counter.swap(tmp);
        }
        int64_t total = 0;
        for (auto &[_, freq] : counter)
            total += freq;
        answer.add(total);
    }
}
AOC_REGISTER_SOLVER(2024, 11, run);

}
