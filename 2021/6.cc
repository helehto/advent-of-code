#include <aoc/base.h>
#include <aoc/string.h>
#include <array>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <string_view>

namespace aoc_2021_6 {

void run(std::string_view buf, aoc::Answer &answer)
{
    std::array<uint64_t, 9> a = {{}};
    for (auto i : find_numbers<uint8_t>(buf))
        a[i - 1]++;

    int i = 0;
    for (; i < 79; i++)
        a[(i + 7) % a.size()] += a[i % a.size()];
    answer.add(std::accumulate(begin(a), end(a), UINT64_C(0)));

    for (; i < 255; i++)
        a[(i + 7) % a.size()] += a[i % a.size()];
    answer.add(std::accumulate(begin(a), end(a), UINT64_C(0)));
}
AOC_REGISTER_SOLVER(2021, 6, run);

}
