#include "common.h"
#include "md5.h"
#include <csignal>
#include <cstring>
#include <future>
#include <thread>
#include <x86intrin.h>

namespace aoc_2015_4 {

static std::pair<int, int>
hash_search(std::string_view s, int n, int stride, std::atomic_int &limit)
{
    int part1 = INT_MAX;
    int part2 = INT_MAX;

    md5::State md5(s);

    for (size_t i = 0; n < limit.load(); i++, n += stride) {
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
    const auto num_threads = std::thread::hardware_concurrency();
    std::vector<std::future<std::pair<int, int>>> futures;

    std::atomic_int limit = INT_MAX;
    for (size_t i = 0; i < num_threads; i++)
        futures.push_back(std::async(std::launch::async, hash_search, buf, 8 * i,
                                     8 * num_threads, std::ref(limit)));

    std::vector<std::pair<int, int>> results;
    for (auto &f : futures)
        results.push_back(f.get());

    int part1 = INT_MAX;
    int part2 = INT_MAX;
    for (auto &[p1, p2] : results) {
        part1 = std::min(part1, p1);
        part2 = std::min(part2, p2);
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
