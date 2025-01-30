#include "common.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2019_20 {

struct alignas(4) State {
    Vec2i8 p;
    int16_t level;

    bool operator==(const State &state) const
    {
        return std::bit_cast<uint32_t>(*this) == std::bit_cast<uint32_t>(state);
    }
};
static_assert(sizeof(State) == 4);

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto g = Matrix<char>::from_lines(lines);

    struct Portal {
        Vec2i8 destination;
        bool inwards;
    };

    Matrix<Portal> portals(g.rows, g.cols);
    dense_map<std::string, Vec2i8> portals_tmp;

    for (Vec2i8 p : g.ndindex<int8_t>()) {
        if (!isalpha(g(p)))
            continue;

        if (auto q = p + Vec2i8{-1, 0}; g.in_bounds(q) && isalpha(g(q)))
            continue;
        if (auto q = p + Vec2i8{0, -1}; g.in_bounds(q) && isalpha(g(q)))
            continue;

        char b[3] = {g(p)};
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
            portals(to) = Portal{it->second, inwards};
            portals(it->second) = Portal{to, !inwards};
        }
    }
    const Vec2i8 start = portals_tmp.at("AA");
    const Vec2i8 end = portals_tmp.at("ZZ");

    dense_set<State, CrcHasher> visited;
    visited.reserve(1'000'000);

    {
        std::vector<State> queue{{start, 0}};
        visited.clear();
        for (size_t i = 0; i < queue.size(); ++i) {
            auto [u, d] = queue[i];
            visited.insert({u});

            if (u == end) {
                fmt::print("{}\n", d);
                break;
            }

            for (auto v : neighbors4(g, u)) {
                if (g(v) == '.' && !visited.count({v}))
                    queue.emplace_back(v, d + 1);
            }

            if (auto [v, inwards] = portals(u); v != Vec2i8{}) {
                if (!visited.count({v}))
                    queue.emplace_back(v, d + 1);
            }
        }
    }

    {
        MonotonicBucketQueue<std::tuple<Vec2i8, int16_t>> queue(2);
        queue.emplace(0, start, 0);
        visited.clear();
        while (auto uu = queue.pop()) {
            const auto [u, level] = *uu;
            visited.insert(State(u, level));

            if (level == 0 && u == end) {
                fmt::print("{}\n", queue.current_priority());
                break;
            }

            for (auto v : neighbors4(g, u)) {
                if (g(v) == '.' && !visited.count(State(v, level)))
                    queue.emplace(queue.current_priority() + 1, v, level);
            }

            if (auto [v, inwards] = portals(u); v != Vec2i8{}) {
                if (level > 0 || inwards) {
                    auto new_level = inwards ? level + 1 : level - 1;
                    if (!visited.count(State(v, new_level)))
                        queue.emplace(queue.current_priority() + 1, v, new_level);
                }
            }
        }
    }
}

}
