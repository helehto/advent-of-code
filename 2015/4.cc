#include "common.h"
#include "md5.h"
#include "thread_pool.h"
#include <cstring>

namespace aoc_2015_4 {

static std::pair<int, int>
hash_search(std::string_view s, int n, int stride, std::atomic_int &limit)
{
    int part1 = INT_MAX;
    int part2 = INT_MAX;

    md5::State md5(s);

    for (; n < limit.load(); n += stride) {
        __m256i hashes = md5.run(n).a;

        if (uint32_t eqmask5 = md5::leading_zero_mask<5>(hashes))
            part1 = std::min(part1, n + std::countr_zero(eqmask5));

        if (uint32_t eqmask6 = md5::leading_zero_mask<6>(hashes)) {
            auto part2 = n + std::countr_zero(eqmask6);
            if (limit.load() >= part2)
                limit.store(part2);
            return std::pair(part1, part2);
        }
    }

    return {part1, part2};
}

void run(std::string_view buf)
{
    ThreadPool &pool = ThreadPool::get();
    std::atomic_int part1 = INT_MAX;
    std::atomic_int part2 = INT_MAX;
    std::atomic_int limit = INT_MAX;

    pool.for_each_thread([&](size_t i) {
        auto [a, b] = hash_search(buf, 8 * i, 8 * pool.num_threads(), limit);
        atomic_store_min(part1, a);
        atomic_store_min(part2, b);
    });

    fmt::print("{}\n", part1.load());
    fmt::print("{}\n", part2.load());
}

}
