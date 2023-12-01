#include "common.h"
#include <fmt/core.h>

void run_2023_1(FILE *f)
{
    int part1 = 0;
    int part2 = 0;

    std::vector<int> v;
    std::string str;
    while (getline(f, str)) {
        std::string_view s = str;

        v.clear();
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] >= '0' && s[i] <= '9')
                v.push_back(s[i] - '0');
        }
        part1 += v.front() * 10 + v.back();

        v.clear();

        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] >= '0' && s[i] <= '9')
                v.push_back(s[i] - '0');
            else if (s.substr(i).starts_with("one"))
                v.push_back(1);
            else if (s.substr(i).starts_with("two"))
                v.push_back(2);
            else if (s.substr(i).starts_with("three"))
                v.push_back(3);
            else if (s.substr(i).starts_with("four"))
                v.push_back(4);
            else if (s.substr(i).starts_with("five"))
                v.push_back(5);
            else if (s.substr(i).starts_with("six"))
                v.push_back(6);
            else if (s.substr(i).starts_with("seven"))
                v.push_back(7);
            else if (s.substr(i).starts_with("eight"))
                v.push_back(8);
            else if (s.substr(i).starts_with("nine"))
                v.push_back(9);
        }

        part2 += v.front() * 10 + v.back();
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
