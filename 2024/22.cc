#include "common.h"
#include "inplace_vector.h"
#include "thread_pool.h"
#include <bitset>

namespace aoc_2024_22 {

void run(std::string_view buf)
{
    ThreadPool &pool = ThreadPool::get();
    auto lines = split_lines(buf);
    std::vector<int> nums;
    constexpr int N = 2000;

    std::vector<inplace_vector<int, N + 1>> secrets;
    secrets.resize(lines.size());

    pool.for_each_index(0, secrets.size(), [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            auto [seed] = find_numbers_n<int, 1>(lines[i]);
            secrets[i].unchecked_push_back(seed);

            while (secrets[i].size() <= N) {
                auto n = secrets[i].back();
                n = ((n << 6) ^ n) & 16777215;
                n = ((n >> 5) ^ n) & 16777215;
                n = ((n << 11) ^ n) & 16777215;
                secrets[i].unchecked_push_back(n);
            }
        }
    });

    fmt::print("{}\n", std::ranges::fold_left(secrets, int64_t(0), λab(a + b[N])));

    std::vector<int16_t> sequence_sum(19 * 19 * 19 * 19);
    std::mutex sequence_sum_mutex;

    pool.for_each_slice(secrets, [&](auto slice) {
        std::vector<int16_t> local_sequence_sum(19 * 19 * 19 * 19);
        std::bitset<19 * 19 * 19 * 19> seen;

        for (const inplace_vector<int, N + 1> &s : slice) {
            seen.reset();

            std::array<int, N + 1> delta;
            for (size_t k = 0; k < delta.size() - 1; ++k)
                delta[k] = s[k] % 10 - s[k + 1] % 10 + 9;

            std::array<int, N + 1> keys;
            for (size_t k = 4; k < delta.size(); ++k)
                keys[k] = 19 * 19 * 19 * delta[k - 4] + 19 * 19 * delta[k - 3] +
                          19 * delta[k - 2] + delta[k - 1];

            for (size_t k = 4; k < keys.size(); ++k) {
                uint32_t key = keys[k];
                if (!seen.test(key)) {
                    seen.set(key);
                    local_sequence_sum[key] += s[k] % 10;
                }
            }
        }

        std::unique_lock lock(sequence_sum_mutex);
        for (size_t i = 0; i < sequence_sum.size(); ++i)
            sequence_sum[i] += local_sequence_sum[i];
    });

    fmt::print("{}\n", std::ranges::max(sequence_sum));
}

}
