#include "common.h"

namespace aoc_2025_8 {

struct UnionFind {
    std::vector<uint16_t> parent;
    std::vector<uint16_t> size;

    constexpr UnionFind(size_t n)
        : parent(n)
        , size(n, 1)
    {
        std::ranges::iota(parent, 0);
    }

    constexpr size_t find(size_t n)
    {
        if (n != parent[n]) {
            return parent[n] = find(parent[n]);
        } else {
            return n;
        }
    }

    constexpr size_t merge(size_t x, size_t y)
    {
        x = find(x);
        y = find(y);

        if (x == y)
            return 0;

        if (size[x] < size[y])
            std::swap(x, y);

        parent[y] = x;
        size[x] += size[y];
        return size[x];
    }
};

constexpr int64_t squared_distance(const std::array<int, 3> &a,
                                   const std::array<int, 3> &b)
{
    const int64_t dx = a[0] - b[0];
    const int64_t dy = a[1] - b[1];
    const int64_t dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

void run(std::string_view buf)
{
    auto nums = find_numbers<int>(buf);
    ASSERT(nums.size() % 3 == 0);
    const auto n = nums.size() / 3;

    auto points = std::make_unique_for_overwrite<std::array<int, 3>[]>(n);
    for (size_t i = 0; i < n; ++i)
        points[i] = {nums[3 * i], nums[3 * i + 1], nums[3 * i + 2]};

    Matrix<int64_t> dist_(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j)
            dist_(i, j) = squared_distance(points[i], points[j]);
    }

    auto pair_dist = [&](std::pair<uint16_t, uint16_t> p) {
        auto [i, j] = p;
        return dist_(std::min(i, j), std::max(i, j));
    };

    std::vector<std::pair<uint16_t, uint16_t>> pairs;
    pairs.reserve(n * (n + 1) / 2);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            pairs.emplace_back(i, j);
    std::ranges::make_heap(pairs, λab(a > b), pair_dist);

    auto pop = [&] {
        std::ranges::pop_heap(pairs, λab(a > b), pair_dist);
        auto result = pairs.back();
        pairs.pop_back();
        return result;
    };

    UnionFind uf(n);

    // Part 1:
    {
        const size_t iterations = n <= 20 ? 10 : 1000;
        for (size_t k = 0; k < iterations; ++k) {
            const auto [i, j] = pop();
            uf.merge(i, j);
        }

        auto s = uf.size;
        std::ranges::partial_sort(s, s.begin() + 3, λab(a > b));
        fmt::print("{}\n", s[0] * s[1] * s[2]);
    }

    // Part 2:
    while (!pairs.empty()) {
        const auto [i, j] = pop();
        if (uf.merge(i, j) == n) {
            fmt::print("{}\n", points[i][0] * points[j][0]);
            break;
        }
    }
}

}
