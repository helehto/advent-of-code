#include "common.h"
#include "inplace_vector.h"

namespace aoc_2017_17 {

static int part1(const int n)
{
    inplace_vector<uint16_t, 2019> value{0};
    inplace_vector<uint16_t, 2019> prev{0};
    inplace_vector<uint16_t, 2019> next{0};
    size_t i = 0;

    for (size_t k = 1; k <= 2017; ++k) {
        auto steps = n % value.size();
        if (steps < value.size()) {
            for (size_t j = 0; j < steps; ++j)
                i = next[i];
        } else {
            for (size_t j = 0; j < value.size() - steps; ++j)
                i = prev[i];
        }

        uint16_t j = value.size();
        value.push_back(k);
        prev.push_back(i);
        next.push_back(next[i]);
        prev[next[i]] = j;
        next[i] = j;
        i = j;
    }
    return value[next[i]];
}

static int part2(const int n)
{
    int result = -1;

    for (int i = 1, next = 0; i <= 50000000; i++) {
        next = (n + next) % i + 1;
        if (next == 1)
            result = i;
    }

    return result;
}

void run(std::string_view buf)
{
    const auto [n] = find_numbers_n<size_t, 1>(buf);
    fmt::print("{}\n", part1(n));
    fmt::print("{}\n", part2(n));
}

}
