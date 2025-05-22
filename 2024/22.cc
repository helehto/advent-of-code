#include "common.h"
#include "inplace_vector.h"
#include <bitset>

namespace aoc_2024_22 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<int> nums;
    constexpr int N = 2000;

    std::vector<inplace_vector<int, N + 1>> secrets;
    secrets.reserve(lines.size());
    for (std::string_view line : lines) {
        auto [n] = find_numbers_n<int, 1>(line);
        auto &s = secrets.emplace_back();
        s.unchecked_push_back(n);

        while (s.size() <= N) {
            auto n = s.back();
            n = ((n << 6) ^ n) & 16777215;
            n = ((n >> 5) ^ n) & 16777215;
            n = ((n << 11) ^ n) & 16777215;
            s.unchecked_push_back(n);
        }
    }

    fmt::print("{}\n", std::ranges::fold_left(secrets, int64_t(0), λab(a + b[N])));

    std::vector<int16_t> sequence_sum(19 * 19 * 19 * 19);
    std::bitset<19 * 19 * 19 * 19> seen;
    for (const inplace_vector<int, N + 1> &s : secrets) {
        seen.reset();

        std::array<int, N + 1> delta;
        for (size_t k = 0; k < delta.size(); ++k)
            delta[k] = s[k] % 10 - s[k + 1] % 10 + 9;

        std::array<int, N + 1> keys;
        for (size_t k = 4; k < delta.size(); ++k)
            keys[k] = 19 * 19 * 19 * delta[k - 4] + 19 * 19 * delta[k - 3] +
                      19 * delta[k - 2] + delta[k - 1];

        for (size_t k = 4; k < keys.size(); ++k) {
            uint32_t key = keys[k];
            if (!seen.test(key)) {
                seen.set(key);
                sequence_sum[key] += s[k] % 10;
            }
        }
    }

    fmt::print("{}\n", std::ranges::max(sequence_sum));
}
}
