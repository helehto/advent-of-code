#include "common.h"
#include "dense_map.h"

namespace aoc_2024_1 {

static int part1(std::span<const int> l, std::span<const int> r)
{
    int result = 0;
    for (size_t i = 0; i < l.size(); ++i)
        result += std::abs(l[i] - r[i]);
    return result;
}

static int part2(std::span<const int> l, std::span<const int> r)
{
    int result = 0;
    dense_map<int, int> freqs;
    freqs.reserve(l.size());
    for (int b : r)
        freqs[b]++;
    for (int a : l) {
        auto it = freqs.find(a);
        result += a * (it != freqs.end() ? it->second : 0);
    }
    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<int> nums;
    std::vector<int> l, r;
    l.reserve(lines.size());
    r.reserve(lines.size());
    for (std::string_view line : lines) {
        find_numbers(line, nums);
        l.push_back(nums[0]);
        r.push_back(nums[1]);
    }

    std::ranges::sort(l);
    std::ranges::sort(r);

    fmt::print("{}\n", part1(l, r));
    fmt::print("{}\n", part2(l, r));
}

}
