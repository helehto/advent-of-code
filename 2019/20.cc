#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2019_20 {

struct alignas(4) State {
    Vec2i8 p;
    int16_t level;
    constexpr bool operator==(const State &state) const = default;
};

constexpr bool xisalpha(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto g = Matrix<char>::from_lines(lines);
    const auto n_letters = std::ranges::count_if(buf, λa(xisalpha(a)));

    struct Portal {
        Vec2i8 destination;
        int16_t level_delta;
    };

    Matrix<Portal> portals(g.rows, g.cols);
    dense_map<std::array<char, 2>, Vec2i8, CrcHasher> portals_tmp;
    dense_map<Vec2i8, Portal> portals_map;
    portals_tmp.reserve(n_letters / 2);
    portals_map.reserve(n_letters / 2);

    for (Vec2i8 p : g.ndindex<int8_t>()) {
        if (!xisalpha(g(p)))
            continue;

        if (auto q = p + Vec2i8{-1, 0}; g.in_bounds(q) && xisalpha(g(q)))
            continue;
        if (auto q = p + Vec2i8{0, -1}; g.in_bounds(q) && xisalpha(g(q)))
            continue;

        std::array<char, 2> b{g(p)};
        Vec2i8 to;
        if (auto q = p + Vec2i8{0, 2}; g.in_bounds(q) && g(q) == '.') {
            b[1] = g(p + Vec2i8{0, 1});
            to = q;
        } else if (auto q = p + Vec2i8{2, 0}; g.in_bounds(q) && g(q) == '.') {
            b[1] = g(p + Vec2i8{1, 0});
            to = q;
        } else if (auto q = p + Vec2i8{-1, 0}; g.in_bounds(q) && g(q) == '.') {
            b[1] = g(p + Vec2i8{1, 0});
            to = q;
        } else if (auto q = p + Vec2i8{0, -1}; g.in_bounds(q) && g(q) == '.') {
            b[1] = g(p + Vec2i8{0, 1});
            to = q;
        } else {
            continue;
        }

        const bool inwards = to.x > 3 && to.x < static_cast<int>(g.cols - 3) &&
                             to.y > 3 && to.y < static_cast<int>(g.rows - 3);
        if (auto [it, inserted] = portals_tmp.emplace(b, to); !inserted) {
            const auto delta = inwards ? +1 : -1;
            portals_map.try_emplace(to, it->second, delta);
            portals_map.try_emplace(it->second, to, -delta);
            portals(to) = Portal(it->second, delta);
            portals(it->second) = Portal(to, -delta);
        }
    }
    const Vec2i8 start = portals_tmp.at({'A', 'A'});
    const Vec2i8 end = portals_tmp.at({'Z', 'Z'});

    portals_map[start] = {};

    auto reachable = [&] {
        dense_map<Vec2i8, small_vector<std::pair<Vec2i8, int>>> result;
        result.reserve(portals.size());
        Matrix<bool> visited(g.rows, g.cols);
        MonotonicBucketQueue<Vec2i8> queue(2);

        for (auto &[pos, _] : portals_map) {
            std::ranges::fill(visited.all(), false);
            queue.clear();
            queue.emplace(0, pos);
            auto &reach = result[pos];

            while (auto u = queue.pop()) {
                visited(*u) = true;

                if (pos != *u && (portals_map.count(*u) || *u == end))
                    reach.emplace_back(*u, queue.current_priority());

                for (auto v : neighbors4(g, *u))
                    if (g(v) == '.' && !visited(v))
                        queue.emplace(queue.current_priority() + 1, v);
            }
        }

        return result;
    }();

    int max_distance = 0;
    for (const auto &[_, v] : reachable)
        for (auto &[_, d] : v)
            max_distance = std::max(max_distance, d);

    MonotonicBucketQueue<State, small_vector<State>> queue(max_distance);
    dense_set<State, CrcHasher> visited;
    visited.reserve(10'000);

    auto solve = [&](const bool allow_recursion) {
        queue.clear();
        visited.clear();
        queue.emplace(0, start, 0);

        while (auto uu = queue.pop()) {
            const auto [u, level] = *uu;

            if (level == 0 && u == end)
                return queue.current_priority();

            for (auto &[v, d] : reachable[u])
                if (visited.emplace(v, level).second)
                    queue.emplace(queue.current_priority() + d, v, level);

            if (auto [v, level_delta] = portals(u); v != Vec2i8{}) {
                auto new_level = allow_recursion ? level + level_delta : level;
                if (new_level >= 0 && visited.emplace(v, new_level).second)
                    queue.emplace(queue.current_priority() + 1, v, new_level);
            }
        }

        ASSERT_MSG(false, "No solution found!?");
    };

    fmt::print("{}\n", solve(false));
    fmt::print("{}\n", solve(true));
}

}
