#include "common.h"
#include "thread_pool.h"
#include <atomic>

namespace aoc_2024_7 {

/// Concatenates the base 10 representations of `a` and `b`. For instance,
/// `concatenate(42, 152)` will return 42152.
constexpr inline uint64_t concatenate(uint64_t a, uint64_t b)
{
    return a * pow10i[digit_count_base10(b)] + b;
}

template <int Part>
constexpr bool solve(int64_t goal, const int64_t *operands, size_t n, int64_t accum = 0)
{
    if (n == 0)
        return accum == goal;
    if (accum > goal)
        return false;

    auto a = operands[0];
    return solve<Part>(goal, operands + 1, n - 1, accum + a) ||
           solve<Part>(goal, operands + 1, n - 1, accum * a) ||
           (Part == 2 && solve<Part>(goal, operands + 1, n - 1, concatenate(accum, a)));
}

void run(std::string_view buf)
{
    ThreadPool &pool = ThreadPool::get();
    auto lines = split_lines(buf);

    std::atomic_int64_t s1 = 0;
    std::atomic_int64_t s2 = 0;
    pool.for_each_slice(lines, [&](auto span) {
        small_vector<int64_t, 16> nums;
        for (std::string_view line : span) {
            find_numbers(line, nums);
            s1.fetch_add(solve<1>(nums[0], &nums[1], nums.size() - 1) ? nums[0] : 0,
                         std::memory_order_relaxed);
            s2.fetch_add(solve<2>(nums[0], &nums[1], nums.size() - 1) ? nums[0] : 0,
                         std::memory_order_relaxed);
        }
    });

    std::atomic_thread_fence(std::memory_order_seq_cst);
    fmt::print("{}\n{}\n", s1.load(), s2.load());
}

}
