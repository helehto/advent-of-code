#include "common.h"
#include "thread_pool.h"
#include <bitset>
#include <mutex>

namespace aoc_2024_22 {

void run(std::string_view buf, aoc::Answer &answer)
{
    std::vector<int> seeds = find_numbers<int>(buf);
    constexpr int N = 2000;

    std::vector<int16_t> sequence_sum(19 * 19 * 19 * 19);
    std::mutex solution_mutex;
    uint64_t secrets_sum = 0;

    ThreadPool::get().for_each_index(0, seeds.size(), [&](size_t begin, size_t end) {
        std::vector<int16_t> local_sequence_sum(19 * 19 * 19 * 19);
        std::bitset<19 * 19 * 19 * 19> seen;

        uint64_t local_secrets_sum = 0;

        for (size_t i = begin; i < end; ++i) {
            seen.reset();

            uint32_t key = 0;
            std::array<int, 4> deltas{}; // sliding 4-element window of deltas
            uint32_t secret = seeds[i];
            int prev_price = seeds[i] % 10;

            auto advance = [&] {
                secret = ((secret << 6) ^ secret) & 16777215;
                secret = ((secret >> 5) ^ secret) & 16777215;
                secret = ((secret << 11) ^ secret) & 16777215;
                const int price = secret % 10;
                const auto delta = prev_price - price + 9;
                key = 19 * (key - 19 * 19 * 19 * deltas[0]) + delta;
                deltas[0] = deltas[1];
                deltas[1] = deltas[2];
                deltas[2] = deltas[3];
                deltas[3] = delta;
                return prev_price = price;
            };

            for (size_t k = 0; k < 3; ++k)
                advance();

            for (size_t k = 3; k < N; ++k) {
                const int price = advance();
                if (!seen[key]) {
                    seen[key] = true;
                    local_sequence_sum[key] += price;
                }
            }
            local_secrets_sum += secret;
        }

        std::unique_lock lock(solution_mutex);
        for (size_t i = 0; i < sequence_sum.size(); ++i)
            sequence_sum[i] += local_sequence_sum[i];
        secrets_sum += local_secrets_sum;
    });

    answer.add(secrets_sum);
    answer.add(std::ranges::max(sequence_sum));
}
AOC_REGISTER_SOLVER(2024, 22, run);

}
