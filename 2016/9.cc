#include "common.h"

namespace aoc_2016_9 {

template <bool Recursive>
static size_t uncompressed_length(std::string_view s)
{
    for (size_t i = 0, result = 0;;) {
        auto j = std::min(s.find('(', i), s.size());
        result += j - i;
        if (j == s.size())
            return result;

        auto k = s.find(')', j);
        ASSERT(k != std::string_view::npos);

        auto [group_len, repeat] = find_numbers_n<int, 2>(s.substr(j, k - j));
        ASSERT(k + group_len <= s.size());

        if constexpr (Recursive) {
            result += repeat * uncompressed_length<Recursive>(s.substr(k + 1, group_len));
        } else {
            result += repeat * group_len;
        }

        i = k + 1 + group_len;
    }
}

void run(std::string_view buf, aoc::Answer &answer)
{
    answer.add(uncompressed_length<false>(buf));
    answer.add(uncompressed_length<true>(buf));
}
AOC_REGISTER_SOLVER(2016, 9, run);

}
