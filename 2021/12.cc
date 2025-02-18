#include "common.h"
#include "dense_map.h"
#include <cctype>

namespace aoc_2021_12 {

static bool is_big(std::string_view s)
{
    for (char c : s)
        if (!(c >= 'A' && c <= 'Z'))
            return false;

    return true;
}

struct SearchParameters {
    std::vector<std::vector<uint8_t>> neighbors;
    uint8_t start_index;
    uint8_t end_index;
    uint64_t big_mask;
    int count = 0;
};

template <bool AllowRevisit>
static void search(SearchParameters &sp,
                   uint8_t current,
                   uint64_t visited_mask,
                   bool any_revisited = false)
{
    if (current == sp.end_index) {
        sp.count++;
        return;
    }

    for (int n : sp.neighbors[current]) {
        uint64_t n_mask = (UINT64_C(1) << n);

        bool is_visited = (visited_mask & n_mask) != 0;
        bool is_revisitable = AllowRevisit && !any_revisited;
        uint64_t new_visited_mask = (visited_mask | n_mask) & ~sp.big_mask;

        if (!is_visited)
            search<AllowRevisit>(sp, n, new_visited_mask, any_revisited);
        else if (is_revisitable)
            search<AllowRevisit>(sp, n, new_visited_mask, true);
    }
}

void run(std::string_view buf)
{
    dense_map<std::string, uint8_t> name_to_index;
    int current_index = 0;

    std::vector<std::pair<uint8_t, uint8_t>> pairs;
    std::vector<std::vector<uint8_t>> neighbors;
    uint64_t big_mask = 0;

    for (std::string_view line : split_lines(buf)) {
        auto dash = line.find('-');
        ASSERT(dash != std::string_view::npos);

        std::string a(line.substr(0, dash));
        std::string b(line.substr(dash + 1));

        int aindex = current_index;
        if (auto [it, inserted] = name_to_index.emplace(a, current_index); inserted)
            current_index++;
        else
            aindex = it->second;

        int bindex = current_index;
        if (auto [it, inserted] = name_to_index.emplace(b, current_index); inserted)
            current_index++;
        else
            bindex = it->second;

        if (is_big(a))
            big_mask |= UINT64_C(1) << aindex;
        if (is_big(b))
            big_mask |= UINT64_C(1) << bindex;

        pairs.emplace_back(aindex, bindex);
    }

    const uint8_t start_index = name_to_index.at("start");
    const uint8_t end_index = name_to_index.at("end");

    neighbors.resize(current_index);
    for (auto &[a, b] : pairs) {
        // Remove all edges to the start cave so that we never visit it again.
        if (b != start_index)
            neighbors[a].push_back(b);
        if (a != start_index)
            neighbors[b].push_back(a);
    }

    {
        SearchParameters sp{neighbors, start_index, end_index, big_mask};
        search<false>(sp, sp.start_index, UINT64_C(1) << sp.start_index);
        fmt::print("{}\n", sp.count);
    }

    {
        SearchParameters sp{neighbors, start_index, end_index, big_mask};
        search<true>(sp, sp.start_index, UINT64_C(1) << sp.start_index);
        fmt::print("{}\n", sp.count);
    }
}

}
