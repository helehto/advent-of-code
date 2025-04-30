#include "common.h"

namespace aoc_2020_15 {

static int32_t step(int32_t turn, int32_t last, std::vector<int32_t> &seen)
{
    const int32_t t = std::exchange(seen[last], turn - 1);
    return t >= 0 ? turn - t - 1 : 0;
}

void run(std::string_view buf)
{
    small_vector<int32_t> init;
    find_numbers(buf, init);

    std::vector<int32_t> seen(30'000'000, -1);
    for (size_t i = 0; i < init.size() - 1; i++)
        seen[init[i]] = i + 1;

    int32_t last = init.back();
    int32_t turn = init.size() + 1;
    for (; turn <= 2020; turn++)
        last = step(turn, last, seen);
    fmt::print("{}\n", last);

    for (; turn <= 30'000'000; turn++)
        last = step(turn, last, seen);
    fmt::print("{}\n", last);
}

}
