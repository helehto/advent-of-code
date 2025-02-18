#include "common.h"

namespace aoc_2024_18 {

void run(std::string_view buf)
{
    std::vector<int> nums;
    find_numbers(buf, nums);

    constexpr int N = 71;
    constexpr Point<uint16_t> start{0, 0};
    constexpr Point<uint16_t> end{N - 1, N - 1};

    Matrix<char> grid(N, N, '.');
    for (size_t i = 0; i < 2 * 1024; i += 2)
        grid(nums[i + 1], nums[i + 0]) = '#';

    std::array<std::vector<Point<uint16_t>>, 1024> queue;
    Matrix<uint16_t> f_score(N, N);
    Matrix<uint16_t> g_score(N, N);

    auto search = [&]() -> int {
        std::ranges::fill(f_score.all(), UINT16_MAX);
        std::ranges::fill(g_score.all(), UINT16_MAX);

        uint16_t d = 0;
        for (auto &bucket : queue)
            bucket.clear();
        queue[0].push_back(start);

        g_score(start) = 0;
        f_score(start) = manhattan(start, end);

        while (true) {
            std::vector<Point<uint16_t>> *bucket;
            size_t i = 0;
            for (; i < queue.size(); i++, d++)
                if (bucket = &queue[d & (queue.size() - 1)], !bucket->empty())
                    break;
            if (i == queue.size())
                return -1;

            auto u = bucket->back();
            bucket->pop_back();

            if (u == end)
                return g_score(end);

            for (auto v : neighbors4(grid, u)) {
                if (grid(v) == '#')
                    continue;

                auto alt = g_score(u) + 1;
                if (alt >= g_score(v))
                    continue;

                g_score(v) = alt;
                f_score(v) = alt + manhattan(v, end);
                d = std::min(d, f_score(v));
                queue[f_score(v) & (queue.size() - 1)].push_back(v);
            }
        }
    };

    fmt::print("{}\n", search());

    size_t lo = 1024;
    size_t hi = nums.size() / 2;

    while (lo + 1 < hi) {
        size_t mid = lo + (hi - lo) / 2;

        std::ranges::fill(grid.all(), '.');
        for (size_t i = 0; i < mid; i++)
            grid(nums[2 * i + 1], nums[2 * i + 0]) = '#';

        if (search() > 0) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    fmt::print("{},{}\n", nums[2 * lo], nums[2 * lo + 1]);
}

}
