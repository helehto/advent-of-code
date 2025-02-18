#include "common.h"

namespace aoc_2023_1 {

constexpr uint64_t pack(std::string_view p)
{
    uint64_t r = 0;
    for (size_t i = 0; i < p.size(); i++)
        r |= static_cast<uint64_t>(p[i]) << (8 * i);
    return r;
}

void run(std::string_view buf)
{
    int part1 = 0;
    int part2 = 0;

    std::vector<int> v;
    for (std::string_view s : split_lines(buf)) {
        v.clear();
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] >= '0' && s[i] <= '9')
                v.push_back(s[i] - '0');
        }
        part1 += v.front() * 10 + v.back();

        v.clear();

        uint64_t window = 0;
        for (size_t i = 0; i < std::min<size_t>(s.size(), 8); i++)
            window |= static_cast<uint64_t>(s[i]) << (8 * i);

        for (size_t i = 0; i < s.size();) {
            if (s[i] >= '0' && s[i] <= '9')
                v.push_back(s[i] - '0');
            else if ((window & 0xffffff) == pack("one"))
                v.push_back(1);
            else if ((window & 0xffffff) == pack("two"))
                v.push_back(2);
            else if ((window & 0xffffffffff) == pack("three"))
                v.push_back(3);
            else if ((window & 0xffffffff) == pack("four"))
                v.push_back(4);
            else if ((window & 0xffffffff) == pack("five"))
                v.push_back(5);
            else if ((window & 0xffffff) == pack("six"))
                v.push_back(6);
            else if ((window & 0xffffffffff) == pack("seven"))
                v.push_back(7);
            else if ((window & 0xffffffffff) == pack("eight"))
                v.push_back(8);
            else if ((window & 0xffffffff) == pack("nine"))
                v.push_back(9);

            i++;
            window = window >> 8;
            if (i + 7 < s.size())
                window |= static_cast<uint64_t>(s[i + 7]) << 56;
        }

        part2 += v.front() * 10 + v.back();
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
