#include "common.h"

namespace aoc_2016_21 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::string s = "abcdefgh";
    for (std::string_view line : lines) {
        std::string old = s;
        if (line.starts_with("reverse")) {
            auto [a, b] = find_numbers_n<int, 2>(line);
            std::reverse(s.begin() + a, s.begin() + b + 1);
        } else if (line.starts_with("move")) {
            auto [from, to] = find_numbers_n<int, 2>(line);

            if (from < to)
                std::rotate(s.begin() + from, s.begin() + from + 1, s.begin() + to + 1);
            else
                std::rotate(s.begin() + to, s.begin() + from, s.begin() + from + 1);
        } else if (line.starts_with("swap position")) {
            auto [a, b] = find_numbers_n<int, 2>(line);
            std::swap(s[a], s[b]);
        } else if (line.starts_with("swap letter")) {
            const size_t i = line.find(" with letter ");
            ASSERT(i != std::string_view::npos);
            std::swap(*std::find(s.begin(), s.end(), line[12]),
                      *std::find(s.begin(), s.end(), line[26]));
        } else if (line.starts_with("rotate left")) {
            auto [n] = find_numbers_n<int, 1>(line);
            std::ranges::rotate(s, s.begin() + n % s.size());
        } else if (line.starts_with("rotate right")) {
            auto [n] = find_numbers_n<int, 1>(line);
            std::ranges::rotate(s, s.end() - n % s.size());
        } else if (line.starts_with("rotate based")) {
            auto i = s.find(line.back());
            ASSERT(i != std::string_view::npos);
            auto n = i + 1 + (i >= 4);
            std::ranges::rotate(s, s.end() - n % s.size());
        }
    }
    fmt::print("{}\n", s);

    s = "fbgdceah";
    for (size_t i = lines.size(); i--;) {
        std::string old = s;
        std::string_view line = lines[i];

        if (line.starts_with("reverse")) {
            auto [a, b] = find_numbers_n<int, 2>(line);
            std::reverse(s.begin() + a, s.begin() + b + 1);
        } else if (line.starts_with("move")) {
            auto [to, from] = find_numbers_n<int, 2>(line);

            if (from < to)
                std::rotate(s.begin() + from, s.begin() + from + 1, s.begin() + to + 1);
            else
                std::rotate(s.begin() + to, s.begin() + from, s.begin() + from + 1);
        } else if (line.starts_with("swap position")) {
            auto [a, b] = find_numbers_n<int, 2>(line);
            std::swap(s[a], s[b]);
        } else if (line.starts_with("swap letter")) {
            const size_t i = line.find(" with letter ");
            ASSERT(i != std::string_view::npos);
            std::swap(*std::find(s.begin(), s.end(), line[12]),
                      *std::find(s.begin(), s.end(), line[26]));
        } else if (line.starts_with("rotate left")) {
            auto [n] = find_numbers_n<int, 1>(line);
            std::ranges::rotate(s, s.end() - n % s.size());
        } else if (line.starts_with("rotate right")) {
            auto [n] = find_numbers_n<int, 1>(line);
            std::ranges::rotate(s, s.begin() + n % s.size());
        } else if (line.starts_with("rotate based")) {
            auto i = s.find(line.back());
            ASSERT(i != std::string_view::npos);

            size_t j = 0;
            for (; j < s.size(); ++j) {
                size_t k = (2 * j + 1 + (j >= 4)) % s.size();
                if (k == i)
                    break;
            }
            ASSERT(j < s.size());

            if (i >= j)
                std::ranges::rotate(s.begin(), s.begin() + i - j, s.end());
            else
                std::ranges::rotate(s.begin(), s.end() + i - j, s.end());
        }
    }

    fmt::print("{}\n", s);
}

}
