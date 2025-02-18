#include "common.h"
#include "dense_map.h"

namespace aoc_2024_11 {

static void step(dense_map<int64_t, int64_t> &counter,
                 dense_map<int64_t, int64_t> &result)
{
    result.clear();

    for (auto [num, count] : counter) {
        if (num == 0) {
            result[1] += count;
        } else if (const auto d = digit_count_base10(num); d % 2 == 0) {
            auto [a, b] = std::div(num, pow10i[digit_count_base10(num) / 2]);
            result[a] += count;
            result[b] += count;
        } else {
            result[num * 2024] += count;
        }
    }
}

void run(std::string_view buf)
{
    dense_map<int64_t, int64_t> counter;
    dense_map<int64_t, int64_t> tmp;
    for (auto n : find_numbers<int64_t>(buf))
        counter[n]++;

    for (int steps : {25, 50}) {
        for (int64_t i = 0; i < steps; i++) {
            step(counter, tmp);
            counter.swap(tmp);
        }
        int64_t total = 0;
        for (auto &[_, freq] : counter)
            total += freq;
        fmt::print("{}\n", total);
    }
}

}
