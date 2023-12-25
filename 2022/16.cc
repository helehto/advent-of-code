#include "common.h"
#include <algorithm>
#include <boost/unordered_map.hpp>
#include <climits>

namespace aoc_2022_16 {

struct Valve {
    int flow = 0;
    uint64_t neighbor_mask = 0;
};

struct Valves {
    std::vector<Valve> valves;
    size_t start_index;
};

constexpr static uint64_t bit(long n)
{
    return UINT64_C(1) << n;
}

// TODO: Clean up this absolute disaster of a function
static Valves parse_valves(FILE *f)
{
    struct ParsedValve {
        std::string name;
        int flow;
        std::vector<std::string> neighbors;
    };

    Valves result;

    boost::unordered_map<std::string, size_t> name_map;
    std::string s;
    size_t i = 0;
    std::vector<ParsedValve> parsed_valves;
    while (getline(f, s)) {
        auto &valve = parsed_valves.emplace_back();

        char name[16];
        int flow;
        sscanf(s.c_str(), "Valve %2[A-Z] has flow rate=%d", name, &flow);

        valve.name = name;
        valve.flow = flow;

        name_map.emplace(name, i);

        std::string_view n = s;
        n.remove_prefix(n.find(';'));
        n.remove_prefix(n.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
        if (!n.empty()) {
            auto j = n.find(", ");
            while (true) {
                valve.neighbors.emplace_back(n.substr(0, j));
                if (j == std::string_view::npos)
                    break;
                n = n.substr(j + 2);
                j = n.find(", ");
            }
        }

        i++;
    }

    for (const auto &pv : parsed_valves) {
        auto &valve = result.valves.emplace_back();
        valve.flow = pv.flow;
        if (pv.name == "AA")
            result.start_index = &pv - &parsed_valves.front();
        for (auto &neighbor_name : pv.neighbors)
            valve.neighbor_mask |= bit(name_map.at(neighbor_name));
    }

    return result;
}

// Construct a matrix (represented as a vector) with the least distances from
// each node to every other node. The distance from 'u' to 'v' is stored in the
// element with index (u * n + v).
static std::vector<int> floyd_warshall(const std::vector<Valve> &valves)
{
    const auto n = valves.size();
    std::vector<int> d(n * n, INT_MAX / 3);

    for (size_t u = 0; u < n; u++) {
        for (size_t i = valves[u].neighbor_mask; i; i &= i - 1) {
            const auto v = __builtin_ctzl(i);
            d[u * n + v] = 1;
        }
    }

    for (size_t i = 0; i < n; i++)
        d[i * n + i] = 0;

    for (size_t k = 0; k < n; k++) {
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                if (d[i * n + j] > d[i + n * k] + d[k * n + j])
                    d[i * n + j] = d[i + n * k] + d[k * n + j];
            }
        }
    }

    return d;
}

struct SearchParameters {
    const Valves &input;
    const std::vector<int> &costs;
    boost::unordered_map<uint64_t, int> &path_scores;
    uint64_t nonzero_mask;
};

struct State {
    size_t u;
    int remaining;
    int score = 0;
    uint64_t visited = 0;
};

// Note: 'visited' is an unordered bitset, so multiple ordered paths may end up
// converging to the same bitset (e.g ('AA', 'BB') and ('BB', 'AA')). This
// doesn't matter though since we are only interested in the maximum, not the
// specific path that was taken.
static void search(const SearchParameters &p, State s)
{
    std::vector<State> stack{s};

    while (!stack.empty()) {
        auto s = stack.back();
        stack.pop_back();

        // Store the score that we would get if we were to stand still in the
        // room from now on (given the current flow).
        if (auto [it, unique] = p.path_scores.emplace(s.visited, s.score); !unique)
            it->second = std::max(it->second, s.score);

        // This mask indicates the nodes that are (still) available for us to
        // visit.
        size_t vmask = ~s.visited & p.nonzero_mask;

        // Step through the available mask bit by bit and recurse.
        for (; vmask; vmask &= vmask - 1) {
            const size_t v = __builtin_ctzl(vmask);
            const auto new_remaining =
                s.remaining - p.costs[s.u * p.input.valves.size() + v] - 1;
            if (new_remaining <= 0)
                continue;

            stack.push_back(State{
                .u = v,
                .remaining = new_remaining,
                .score = s.score + p.input.valves[v].flow * new_remaining,
                .visited = s.visited | bit(v),
            });
        }
    }
}

void run(FILE *f)
{
    const auto input = parse_valves(f);
    const auto costs = floyd_warshall(input.valves);

    // Build a bitmask of nodes with non-zero flow. This allows search() to
    // more quickly ignore them.
    uint64_t nonzero_mask = 0;
    for (size_t i = 0; i < input.valves.size(); i++) {
        if (input.valves[i].flow != 0)
            nonzero_mask |= bit(i);
    }

    // Part 1:
    {
        boost::unordered_map<uint64_t, int> path_scores;
        SearchParameters p{input, costs, path_scores, nonzero_mask};
        search(p, State{.u = input.start_index, .remaining = 30});
        int m = 0;
        for (auto &[_, score] : path_scores)
            m = std::max(m, score);
        fmt::print("{}\n", m);
    }

    // Part 2:
    {
        boost::unordered_map<uint64_t, int> path_scores;
        SearchParameters p{input, costs, path_scores, nonzero_mask};
        search(p, State{.u = input.start_index, .remaining = 26});

        // Sorting the paths by descending score allows for some short
        // circuiting below when finding disjoint paths with the maximum sum.
        std::vector<std::pair<uint64_t, int>> sorted_scores(begin(path_scores),
                                                            end(path_scores));
        std::sort(sorted_scores.begin(), sorted_scores.end(),
                  [](const auto &a, const auto &b) { return a.second > b.second; });

        int score = 0;
        for (size_t i = 1; i < sorted_scores.size(); i++) {
            const auto [p1, v1] = sorted_scores[i];
            if (v1 + sorted_scores[0].second <= score)
                break;
            for (size_t j = i + 1; j < sorted_scores.size(); j++) {
                const auto [p2, v2] = sorted_scores[j];
                if (v1 + v2 <= score)
                    break;
                if ((p1 & p2) == 0) {
                    score = v1 + v2;
                    break;
                }
            }
        }

        fmt::print("{}\n", score);
    }
}

}
