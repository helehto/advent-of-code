#include "common.h"
#include <optional>

namespace aoc_2023_5 {

struct Range {
    uint64_t to;
    uint64_t from;
    uint64_t n;
};

static std::pair<uint64_t, uint64_t>
intersect(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if (bs >= ae || as >= be)
        return {0, 0};

    return std::pair(std::max(as, bs), std::min(ae, be));
}

static void search(std::span<const std::vector<Range>> tables,
                   size_t index,
                   uint64_t start,
                   uint64_t end,
                   uint64_t &minimum)
{
    if (index >= tables.size()) {
        minimum = std::min(minimum, start);
        return;
    }
    auto &tab = tables[index];

    // Handle entries in the range prior to the first entry in the table.
    if (auto [a, b] = intersect(0, tab[0].from, start, end); a != b)
        search(tables, index + 1, a, b, minimum);

    // Handle entries in the range past the last entry in the table.
    auto table_end = tab.back().from + tab.back().n;
    if (auto [a, b] = intersect(table_end, UINT64_MAX, start, end); a != b)
        search(tables, index + 1, a, b, minimum);

    for (size_t i = 0; i < tab.size(); i++) {
        // Try remapping (part of) the range using this table entry.
        const auto [to, from, n] = tab[i];
        if (auto [a, b] = intersect(from, from + n, start, end); a != b)
            search(tables, index + 1, a - from + to, b - from + to, minimum);

        // Try identity mapping the part of the range that lies between this
        // and the next table entry. (Two adjacent table entries might not
        // leave any space between them, so explicitly check for that.)
        if (i != tab.size() - 1 && from + n != tab[i + 1].from) {
            if (auto [a, b] = intersect(from + n, tab[i + 1].from, start, end); a != b)
                search(tables, index + 1, a, b, minimum);
        }
    }
}

void run(FILE *f)
{
    std::string s;
    getline(f, s);

    auto seeds = find_numbers<uint64_t>(s);

    std::vector<uint64_t> nums;
    std::vector<Range> tables[7];
    getline(f, s);
    for (size_t i = 0; getline(f, s); i++) {
        while (getline(f, s) && !s.empty()) {
            find_numbers(s, nums);
            tables[i].emplace_back(nums[0], nums[1], nums[2]);
        }
    }

    for (auto &table : tables)
        std::sort(table.begin(), table.end(),
                  [](auto &a, auto &b) { return a.from < b.from; });

    uint64_t part1 = UINT64_MAX;
    for (auto seed : seeds)
        search(tables, 0, seed, seed + 1, part1);
    fmt::print("{}\n", part1);

    uint64_t part2 = UINT64_MAX;
    for (size_t i = 0; i < seeds.size(); i += 2)
        search(tables, 0, seeds[i], seeds[i] + seeds[i + 1], part2);
    fmt::print("{}\n", part2);
}

}
