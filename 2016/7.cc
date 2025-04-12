#include "common.h"

namespace aoc_2016_7 {

static bool contains_abba(std::string_view s)
{
    for (size_t i = 3; i < s.size(); ++i)
        if (s[i - 3] == s[i] && s[i - 2] == s[i - 1] && s[i - 3] != s[i - 2])
            return true;
    return false;
}

static int part1(const std::vector<std::string_view> &lines)
{
    int s = 0;

    std::vector<std::string_view> segments;

    for (std::string_view line : lines) {
        split(line, segments, λx(x == '[' || x == ']'));

        for (size_t i = 1; i < segments.size(); i += 2)
            if (contains_abba(segments[i]))
                goto done;
        for (size_t i = 0; i < segments.size(); i += 2) {
            if (contains_abba(segments[i])) {
                s++;
                break;
            }
        }

    done:;
    }

    return s;
}

static int part2(const std::vector<std::string_view> &lines)
{
    std::vector<bool> aba;
    std::vector<bool> bab;
    int s = 0;

    std::vector<std::string_view> segments;

    for (std::string_view line : lines) {
        split(line, segments, λx(x == '[' || x == ']'));

        aba.assign(32 * 32, false);
        bab.assign(32 * 32, false);

        for (size_t i = 0; i < segments.size(); ++i) {
            std::string_view seg = segments[i];
            for (size_t j = 2; j < seg.size(); ++j) {
                if (seg[j - 2] != seg[j])
                    continue;

                size_t aba_index = 32 * (seg[j] - 'a') + seg[j - 1] - 'a';
                size_t bab_index = 32 * (seg[j - 1] - 'a') + seg[j] - 'a';

                if (i % 2 == 0) {
                    if (bab[aba_index]) {
                        s++;
                        goto done;
                    }
                    aba[aba_index] = true;
                } else {
                    if (aba[bab_index]) {
                        s++;
                        goto done;
                    }
                    bab[bab_index] = true;
                }
            }
        }

    done:;
    }

    return s;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    fmt::print("{}\n", part1(lines));
    fmt::print("{}\n", part2(lines));
}

}
