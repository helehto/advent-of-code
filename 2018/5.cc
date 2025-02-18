#include "common.h"

namespace aoc_2018_5 {

static size_t react(std::string &polymer, std::string_view input)
{
    polymer.clear();

    for (char c : input) {
        if (!polymer.empty() && (polymer.back() ^ 0x20) == c)
            polymer.pop_back();
        else
            polymer.push_back(c);
    }

    return polymer.size();
}

void run(std::string_view buf)
{
    auto s = split_lines(buf)[0];

    std::string result;
    fmt::print("{}\n", react(result, {s}));

    size_t min = SIZE_MAX;
    std::string spliced;
    spliced.reserve(s.size());
    for (char c = 'a'; c <= 'z'; c++) {
        spliced.clear();
        for (char k : s)
            if ((k | 0x20) != c)
                spliced.push_back(k);
        min = std::min(min, react(result, spliced));
    }
    fmt::print("{}\n", min);
}

}
