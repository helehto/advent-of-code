#include "common.h"
#include <regex>

namespace aoc_2024_3 {

static int match_product(std::string_view s, const std::match_results<const char *> &m)
{
    std::string_view a_match(s.data() + m.position(1), m.length(1));
    std::string_view b_match(s.data() + m.position(2), m.length(2));

    int a = 0;
    int b = 0;
    std::from_chars(a_match.data(), a_match.data() + a_match.size(), a);
    std::from_chars(b_match.data(), b_match.data() + b_match.size(), b);
    return a * b;
}

static int part1(std::string_view s)
{
    std::regex re(R"(mul\((\d+),(\d+)\))");
    auto it = std::cregex_iterator(s.begin(), s.end(), re);
    auto end = std::cregex_iterator();

    int result = 0;
    for (; it != end; ++it)
        result += match_product(s, *it);
    return result;
}

static int part2(std::string_view s)
{
    std::regex re(R"(mul\((\d+),(\d+)\)|do\(\)|don't\(\))");
    auto it = std::cregex_iterator(s.begin(), s.end(), re);
    auto end = std::cregex_iterator();

    int result = 0;
    bool enabled = true;
    for (; it != end; ++it) {
        std::string_view text(s.data() + it->position(0), it->length(0));
        if (text.starts_with("don't"))
            enabled = false;
        else if (text.starts_with("do"))
            enabled = true;
        else if (enabled)
            result += match_product(s, *it);
    }
    return result;
}

void run(std::string_view buf)
{
    fmt::print("{}\n", part1(buf));
    fmt::print("{}\n", part2(buf));
}

}
