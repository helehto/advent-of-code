#include "common.h"

namespace aoc_2023_6 {

static uint64_t ways(uint64_t t, uint64_t d)
{
    return std::floor(t / 2. + sqrt(t * t / 4 - d) - 1e-6) -
           std::ceil(t / 2. - sqrt(t * t / 4 - d) + 1e-6) + 1;
}

void run(FILE *f)
{
    std::string s;
    getline(f, s);
    auto ts = find_numbers<int>(s);
    getline(f, s);
    auto ds = find_numbers<int>(s);

    int prod = 1;
    for (size_t i = 0; i < ts.size(); i++)
        prod *= ways(ts[i], ds[i]);

    uint64_t t = 0;
    uint64_t d = 0;
    for (size_t i = 0; i < ts.size(); i++) {
        t = pow(10, ceil(log10(ts[i]))) * t + ts[i];
        d = pow(10, ceil(log10(ds[i]))) * d + ds[i];
    }
    fmt::print("{}\n{}\n", prod, ways(t, d));
}

}
