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
    for (std::string_view line : split_lines(buf)) {
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

    int64_t s1 = 0;
    for (auto &s : secrets)
        s1 += s[N];
    fmt::print("{}\n", s1);

    std::vector<inplace_vector<int, N>> prices;
    prices.reserve(lines.size());
    for (auto &s : secrets) {
        auto &p = prices.emplace_back();
        for (size_t i = 1; i < s.size(); i++)
            p.unchecked_push_back(s[i] % 10 - s[i - 1] % 10 + 9);
    }

    std::vector<std::vector<int>> sequences(19 * 19 * 19 * 19);
    std::bitset<19 * 19 * 19 * 19> seen;
    for (size_t i = 0; i < secrets.size(); i++) {
        seen.reset();
        const auto &s = secrets[i];
        const auto &p = prices[i];

        for (size_t j = 4; j < p.size(); ++j) {
            uint32_t key = 19 * (19 * (19 * p[j - 4] + p[j - 3]) + p[j - 2]) + p[j - 1];
            if (!seen.test(key)) {
                seen.set(key);
                sequences[key].push_back(s[j] % 10);
            }
        }
    }

    std::vector<int64_t> q;
    for (auto &seq : sequences) {
        int64_t t = 0;
        for (auto v : seq)
            t += v;
        q.push_back(t);
    }
    fmt::print("{}\n", *std::max_element(q.begin(), q.end()));
}

}
