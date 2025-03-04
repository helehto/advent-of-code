#include "common.h"
#include "dense_map.h"

namespace aoc_2017_3 {

static Vec2i num_to_xy(const int n)
{
    const int d = std::floor(std::sqrt(n));

    Vec2i delta{};
    if (n != d * d) {
        if (n <= d * d + d + 1)
            delta = Vec2i{1, n - d * d - 1};
        else
            delta = Vec2i{(d * d + d + 1) - n + 1, d};
    }

    if (d % 2 == 0) {
        Vec2i p{-(d - 1) / 2, d / 2};
        return p - delta;
    } else {
        Vec2i p{d / 2, -d / 2};
        return p + delta;
    }
}

static int part2(const int n)
{
    dense_map<Vec2i, int> g;
    g.reserve(100);
    g.emplace(Vec2i{0, 0}, 1);

    for (int k = 2;; ++k) {
        auto p = num_to_xy(k);

        int sum = 0;
        for (auto q : neighbors8(p)) {
            if (auto it = g.find(q); it != g.end())
                sum += it->second;
        }

        if (sum > n)
            return sum;

        ASSERT(g.emplace(p, sum).second);
    }
}

void run(std::string_view buf)
{
    auto [n] = find_numbers_n<int, 1>(buf);
    fmt::print("{}\n", manhattan(num_to_xy(n)));
    fmt::print("{}\n", part2(n));
}

}
