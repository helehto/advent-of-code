#include "common.h"
#include "dense_map.h"
#include <numeric>
#include <tuple>

namespace aoc_2022_24 {

enum { U, D, L, R };

struct Grid {
    std::vector<uint8_t> grid[4];
    int m;
    int n;

    Grid(int m, int n)
        : m(m)
        , n(n)
    {
        for (auto &g : grid)
            g.resize(n * m);
    }

    constexpr bool at(int dir, Vec2i p) const { return grid[dir][p.y * m + p.x]; }

    constexpr bool is_unoccupied(Vec2i p, int t) const
    {
        return !at(U, {p.x, (p.y - 1 + t) % (n - 2) + 1}) &&
               !at(D, {p.x, modulo(p.y - 1 - t, n - 2) + 1}) &&
               !at(L, {(p.x - 1 + t) % (m - 2) + 1, p.y}) &&
               !at(R, {modulo(p.x - 1 - t, m - 2) + 1, p.y});
    }

    auto get_moves(Vec2i v, Vec2i goal, int t) const
    {
        inplace_vector<Vec2i, 6> result;

        if (is_unoccupied(v, t))
            result.push_back(v);

        for (auto w : neighbors4(v)) {
            if (w == goal || (0 < w.x && w.x < m - 1 && 0 < w.y && w.y < n - 1 &&
                              is_unoccupied(w, t))) {
                result.push_back(w);
            }
        }

        return result;
    }
};

struct QueueEntry {
    Vec2i v;
    int heuristic;
    int time;

    constexpr bool operator==(const QueueEntry &) const = default;
};

struct GMapEntry {
    Vec2i start;
    int time;

    constexpr bool operator==(const GMapEntry &) const = default;
};

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    const size_t n = lines.size();
    const size_t m = lines[0].size();

    Grid grid(m, n);

    for (size_t y = 1; y < lines.size() - 1; y++) {
        const auto &line = lines[y];
        for (size_t x = 1; x < line.size() - 1; x++) {
            if (line[x] == '^')
                grid.grid[U][y * m + x] = true;
            if (line[x] == 'v')
                grid.grid[D][y * m + x] = true;
            if (line[x] == '<')
                grid.grid[L][y * m + x] = true;
            if (line[x] == '>')
                grid.grid[R][y * m + x] = true;
        }
    }

    dense_map<GMapEntry, int, CrcHasher> g;
    auto search = [&](int t, Vec2i start, Vec2i goal) -> std::tuple<int, int> {
        g.clear();
        g.insert({{start, t}, 0});
        std::vector<QueueEntry> q{{start, manhattan(start, goal), t}};

        while (!q.empty()) {
            auto heap_cmp = λab(a.heuristic > b.heuristic);
            std::pop_heap(begin(q), end(q), heap_cmp);
            auto e = q.back();
            q.pop_back();

            if (e.v == goal)
                return std::tuple(g[{e.v, e.time}], e.time);

            for (auto w : grid.get_moves(e.v, goal, e.time + 1)) {

                int tentative = INT_MAX / 2;
                if (auto it = g.find({e.v, e.time}); it != g.end())
                    tentative = it->second + 1;

                GMapEntry wh{w, e.time + 1};
                if (auto [it, inserted] = g.emplace(wh, tentative);
                    !inserted && tentative >= it->second)
                    continue;

                q.push_back({w, tentative + manhattan(w, goal), e.time + 1});
                std::push_heap(begin(q), end(q), heap_cmp);
            }
        }

        __builtin_trap();
    };

    Vec2i start_p(1, 0);
    Vec2i goal_p(lines.back().find("."), n - 1);
    auto [cost1, t1] = search(0, start_p, goal_p);
    auto [cost2, t2] = search(t1, start_p, goal_p);
    auto [cost3, t3] = search(t2, start_p, goal_p);
    fmt::print("{}\n", cost1);
    fmt::print("{}\n", cost1 + cost2 + cost3);
}

}
