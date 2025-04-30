#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2017_7 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    dense_map<std::string_view, int> name_to_index;
    name_to_index.reserve(lines.size());
    std::vector<std::string_view> names(lines.size());
    std::vector<int> local_weight(lines.size());
    for (size_t i = 0; std::string_view line : lines) {
        const auto name = line.substr(0, line.find(' '));
        names[i] = name;
        name_to_index.emplace(name, i);
        local_weight[i] = find_numbers_n<int, 1>(line)[0];
        ++i;
    }

    std::vector<small_vector<int16_t, 2>> above(names.size());
    std::vector<int16_t> below(names.size(), -1);
    std::vector<std::string_view> tmp;

    for (size_t i = 0; std::string_view line : lines) {
        if (auto arrow = line.find("->"); arrow != std::string_view::npos) {
            for (std::string_view v : split(line.substr(arrow + 3), tmp, ',')) {
                size_t j = name_to_index.at(strip(v));
                above[i].push_back(j);
                ASSERT(below[j] < 0);
                below[j] = i;
            }
        }

        ++i;
    }

    for (size_t i = 0; i < below.size(); ++i) {
        if (below[i] < 0) {
            fmt::print("{}\n", names[i]);
            break;
        }
    }

    // Find top programs to fill the initial queue for topological sort below:
    std::vector<int16_t> topo;
    std::vector<uint8_t> n_above(below.size());
    topo.reserve(above.size());
    for (size_t i = 0; i < above.size(); ++i) {
        n_above[i] = above[i].size();
        if (above[i].empty())
            topo.push_back(i);
    }

    // Topological sort and accumulating weights:
    std::vector<int32_t> cumulative_weight = local_weight;
    for (size_t i = 0; i < topo.size(); ++i) {
        auto u = topo[i];
        for (auto v : above[u])
            cumulative_weight[u] += cumulative_weight[v];
        if (auto v = below[u]; v >= 0 && !--n_above[v])
            topo.push_back(v);
    }

    // Find the first bad program.
    size_t bad = -1;
    dense_map<int, int> counter;
    for (size_t u : topo) {
        if (!above[u].empty()) {
            counter.clear();
            for (size_t v : above[u])
                ++counter[cumulative_weight[v]];
            if (counter.size() == 2) {
                bad = u;
                break;
            }
        }
    }
    ASSERT(bad < names.size());

    small_vector<std::pair<int, int>> r;
    for (int16_t u : above[bad])
        r.emplace_back(cumulative_weight[u], cumulative_weight[u] - local_weight[u]);
    ASSERT(!r.empty());
    std::ranges::sort(r);
    fmt::print("{}\n", r[0].first != r[1].first
                           ? r[1].first - r[0].second
                           : r[r.size() - 2].first - r[r.size() - 1].second);
}

}
