#include "common.h"
#include "thread_pool.h"

namespace aoc_2017_24 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    ASSERT(lines.size() < 64);

    inplace_vector<std::array<uint8_t, 2>, 64> components;
    for (std::string_view line : lines)
        components.unchecked_push_back(find_numbers_n<uint8_t, 2>(line));

    std::array<inplace_vector<std::pair<uint8_t, uint8_t>, 64>, 64> compatible;
    for (size_t i = 0; i < 64; ++i) {
        for (size_t j = 0; j < components.size(); ++j) {
            auto [a, b] = components[j];
            if (i == a)
                compatible[i].unchecked_emplace_back(j, b);
            if (i == b)
                compatible[i].unchecked_emplace_back(j, a);
        }
    }

    struct alignas(16) Entry {
        uint64_t remaining;
        uint16_t curr;
        uint16_t depth;
        int32_t weight;
    };

    struct alignas(8) Part2Result {
        int32_t depth;
        int32_t weight;
        std::strong_ordering operator<=>(const Part2Result &other) const = default;
    };

    ThreadPool &pool = ThreadPool::get();
    ForkPool<Entry> fork_pool(pool.num_threads());
    fork_pool.push({Entry{UINT64_MAX >> (64 - components.size()), 0, 0, 0}});

    // TODO: This is ugly as sin.
    struct alignas(64) PerThreadResult {
        int32_t part1 = 0;
        Part2Result part2{0, 0};
    };
    std::vector<PerThreadResult> thread_max(pool.num_threads());

    fork_pool.run(pool, [&](ForkPool<Entry>::TaskContext &ctx, const Entry &e) {
        auto [remaining, curr, depth, weight] = e;
        weight += curr;

        auto &[p1, p2] = thread_max[ctx.thread_id];
        p1 = std::max(p1, weight);
        p2 = std::max(p2, Part2Result(depth, weight));

        for (const auto &[idx, other] : compatible[curr]) {
            const uint64_t bit = UINT64_C(1) << idx;
            if (remaining & bit)
                ctx.next.emplace_back(remaining & ~bit, other, depth + 1, weight + curr);
        }
    });

    fmt::print("{}\n", std::ranges::max(thread_max, {}, λa(a.part1)).part1);
    fmt::print("{}\n", std::ranges::max(thread_max, {}, λa(a.part2)).part2.weight);
}

}
