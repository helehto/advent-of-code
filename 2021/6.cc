#include "common.h"
#include <fmt/core.h>
#include <numeric>
#include <span>
#include <vector>

void run_2021_6(FILE *f)
{
    std::array<uint64_t, 9> a = {{}};
    std::string s;
    getline(f, s);
    for (auto i : find_numbers<uint8_t>(s))
        a[i - 1]++;

    int i = 0;
    for (; i < 79; i++)
        a[(i + 7) % a.size()] += a[i % a.size()];
    fmt::print("{}\n", std::accumulate(begin(a), end(a), UINT64_C(0)));

    for (; i < 255; i++)
        a[(i + 7) % a.size()] += a[i % a.size()];
    fmt::print("{}\n", std::accumulate(begin(a), end(a), UINT64_C(0)));
}
