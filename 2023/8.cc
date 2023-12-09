#include "common.h"
#include <fmt/ranges.h>
#include <numeric>
#include <unordered_map>

void run_2023_8(FILE *f)
{
    std::string directions;
    getline(f, directions);
    std::string unused;
    getline(f, unused);
    auto lines = getlines(f);

    std::vector<std::pair<uint16_t, uint16_t>> next(lines.size());
    std::unordered_map<std::string_view, uint16_t> name_to_index_map;

    for (size_t i = 0; std::string_view s : lines) {
        auto index_from_name = [&](std::string_view sv) {
            auto [it, inserted] = name_to_index_map.emplace(sv, i);
            return inserted ? i++ : it->second;
        };

        const size_t ai = index_from_name(s.substr(0, 3));
        const size_t bi = index_from_name(s.substr(7, 3));
        const size_t ci = index_from_name(s.substr(12, 3));
        next[ai] = {bi, ci};
    }

    std::vector<size_t> starts;
    size_t aaa = 0;
    size_t zzz = 0;
    for (const auto &[k, i] : name_to_index_map) {
        if (k == "AAA")
            aaa = i;
        if (k == "ZZZ")
            zzz = i;
        if (k.ends_with("A"))
            starts.push_back(i);
    }

    struct State8 {
        size_t curr;
        size_t dir_index = 0;

        bool operator==(const State8 &) const = default;
    };

    auto step = [&](State8 s) {
        State8 new_state;
        new_state.curr =
            directions[s.dir_index] == 'L' ? next[s.curr].first : next[s.curr].second;
        new_state.dir_index = s.dir_index + 1;
        if (new_state.dir_index >= directions.size())
            new_state.dir_index -= directions.size();
        return new_state;
    };

    auto s1 = step({aaa});
    size_t steps = 1;
    for (; s1.curr != zzz; steps++)
        s1 = step(s1);
    fmt::print("{}\n", steps);

    uint64_t part2 = 1;
    for (size_t k : starts) {
        int cycle_length = 1;
        State8 slow{k};
        State8 fast = step(slow);

        // Brent's cycle detection algorithm
        for (int power = 1; slow != fast; cycle_length++) {
            if (power == cycle_length) {
                slow = fast;
                power *= 2;
                cycle_length = 0;
            }
            fast = step(fast);
        }

        part2 = std::lcm(part2, cycle_length);
    }
    fmt::print("{}\n", part2);
}
