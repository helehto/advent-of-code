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

void run(std::string_view buf)
{
    auto nums = find_numbers<uint32_t>(buf);
    auto lines = split_lines(buf);
    const size_t n = lines.size();
    auto input_storage = std::make_unique_for_overwrite<float[]>(3 * lines.size());
    std::span xs(input_storage.get(), n);
    std::span ys(input_storage.get() + n, n);
    std::span zs(input_storage.get() + 2 * n, n);

    for (size_t i = 0; i < n; ++i) {
        auto [x, y, z] = find_numbers_n<uint32_t, 3>(lines[i]);
        xs[i] = x;
        ys[i] = y;
        zs[i] = z;
    }

    // TODO: Is it feasible to store just a packed upper-triangular matrix for
    // better cache locality?
    Matrix<float> dist_(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const float dx = xs[i] - xs[j];
            const float dy = ys[i] - ys[j];
            const float dz = zs[i] - zs[j];
            dist_(i, j) = dx * dx + dy * dy + dz * dz;
        }
    }

    auto pair_dist = [&](std::pair<uint16_t, uint16_t> p) {
        auto [i, j] = p;
        return dist_(std::min(i, j), std::max(i, j));
    };

    constexpr float bucket_width = 4096;

    const size_t part1_iterations = n <= 20 ? 10 : 1000;
    UnionFind uf(n);
    size_t iters = 0;
    auto bucket_storage =
        std::make_unique_for_overwrite<std::pair<uint16_t, uint16_t>[]>(n * (n - 1) / 2);
    size_t bucket_len = 0;
    for (size_t y = 0;; ++y) {
        bucket_len = 0;

        // The general idea here is that we don't want to sort *all* pairs by
        // distances up front, since there are Θ(n^2) of them. We will form a
        // connected graph long before all pairs are exhausted, so the work of
        // sorting all pairs after the last one used would be pointless.
        //
        // On the other hand, we don't know beforehand how many pairs that are
        // needed to form a connected graph, so we can't just call
        // std::partial_sort() with some fixed argument and call it a day
        // either. (Computing this is more or less as expensive as just solving
        // the original problem.)
        //
        // Instead, gather pairs which lie inside the interval [lo, hi) for
        // each iteration, with the interval incrementing by `bucket_width`
        // each time, and gather and sort only those.
        const float lo = y * bucket_width;
        const float hi = (y + 1) * bucket_width;
        const __m256 vlo2 = _mm256_set1_ps(lo * lo);
        const __m256 vhi2 = _mm256_set1_ps(hi * hi);

        // Form a bucket by collecting all pairs with distances in [lo, hi).
        //
        // TODO: Would partition() or nth_element() be faster here to avoid
        // re-scanning pairs we have looked at in future iterations of the
        // outer loop?
        for (size_t i = 0; i < n; ++i) {
            const float *row = dist_.row(i).data();

            size_t j = i + 1;

            for (; j + 7 < n; j += 8) {
                const __m256 vd2 = _mm256_loadu_ps(&row[j]);
                const __m256 vcmp_lo = _mm256_cmp_ps(vd2, vlo2, _CMP_GE_OQ);
                const __m256 vcmp_hi = _mm256_cmp_ps(vd2, vhi2, _CMP_LT_OQ);
                const __m256 vmask = _mm256_and_ps(vcmp_lo, vcmp_hi);
                const int mask = _mm256_movemask_ps(vmask);
                for (uint32_t m = mask; m; m &= m - 1) {
                    const int bit = std::countr_zero(m);
                    bucket_storage[bucket_len++] = {static_cast<uint16_t>(i),
                                                    static_cast<uint16_t>(j + bit)};
                }
            }

            for (; j < n; ++j) {
                if (const auto d = row[j]; d >= lo * lo && d < hi * hi) {
                    bucket_storage[bucket_len++] = {static_cast<uint16_t>(i),
                                                    static_cast<uint16_t>(j)};
                }
            }
        }

        // Sort the bucket by distance.
        std::span bucket(bucket_storage.get(), bucket_len);
        std::ranges::sort(bucket, λab(a < b), pair_dist);

        for (auto &[i, j] : bucket) {
            if (uf.merge(i, j) == n) {
                // Network is fully connected; part 2 done.
                const int64_t xi = xs[i];
                const int64_t xj = xs[j];
                fmt::print("{}\n", xi * xj);
                return;
            }
            if (iters == part1_iterations) {
                auto s = uf.size;
                std::ranges::partial_sort(s, s.begin() + 3, λab(a > b));
                fmt::print("{}\n", s[0] * s[1] * s[2]);
            }
            iters++;
        }
    }

    ASSERT(false);
}

}
