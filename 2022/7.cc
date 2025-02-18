#include "common.h"
#include <climits>
#include <unordered_map>

namespace aoc_2022_7 {

void run(std::string_view buf)
{
    std::unordered_map<std::string, int> sizes;
    std::vector<std::string_view> cwd;
    std::string path;

    for (std::string_view s : split_lines(buf)) {
        if (s == "$ cd /") {
            cwd = {""};
        } else if (s == "$ cd ..") {
            cwd.pop_back();
        } else if (s.starts_with("$ cd ")) {
            cwd.push_back(s.substr(5));
        } else if (isdigit(s[0])) {
            auto i = s.find(' ');
            int size = 0;
            std::from_chars(&s[0], &s[i], size);
            sizes[""] += size;
            path.clear();
            for (size_t i = 1; i < cwd.size(); i++) {
                path += "/";
                path += cwd[i];
                sizes[path] += size;
            }
        }
    }

    const int needed = sizes[""] - 40'000'000;
    int part1 = 0;
    int part2 = INT_MAX;
    for (auto &[_, v] : sizes) {
        if (v <= 100'000)
            part1 += v;
        if (v >= needed)
            part2 = std::min(part2, v);
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
