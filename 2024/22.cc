#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2024_22 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    std::vector<int> nums;

    std::vector<std::vector<int>> secrets;
    for (std::string_view line : lines) {
        find_numbers(line, nums);

        std::vector<int> s;
        s.reserve(2002);
        s.push_back(nums[0]);

        while (s.size() <= 2001) {
            auto n = s.back();
            n = ((n << 6) ^ n) & 16777215;
            n = ((n >> 5) ^ n) & 16777215;
            n = ((n << 11) ^ n) & 16777215;
            s.push_back(n);
        }

        secrets.push_back(std::move(s));
    }

    int64_t s1 = 0;
    for (auto &s : secrets)
        s1 += s[2000];
    fmt::print("{}\n", s1);

    std::vector<std::vector<int>> prices;

    for (auto &s : secrets) {
        std::vector<int> p;
        p.reserve(s.size() - 1);
        for (size_t i = 1; i < s.size(); i++)
            p.push_back(s[i] % 10 - s[i - 1] % 10);
        prices.push_back(p);
    }

    dense_map<std::tuple<int, int, int, int>, std::vector<int>, TuplelikeHasher>
        sequences;
    dense_set<std::tuple<int, int, int, int>, TuplelikeHasher> seen;
    for (size_t i = 0; i < secrets.size(); i++) {
        seen.clear();
        const auto &s = secrets[i];
        const auto &p = prices[i];

        for (size_t j = 4; j < p.size(); ++j) {
            std::tuple seq(p[j - 4], p[j - 3], p[j - 2], p[j - 1]);
            if (auto [_, inserted] = seen.insert(seq); inserted)
                sequences[seq].push_back(s[j] % 10);
        }
    }

    std::vector<int64_t> q;
    for (auto &[_, seq] : sequences) {
        int64_t t = 0;
        for (auto v : seq)
            t += v;
        q.push_back(t);
    }
    fmt::print("{}\n", *std::max_element(q.begin(), q.end()));
}

}
