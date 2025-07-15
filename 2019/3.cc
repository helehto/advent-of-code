#include "common.h"
#include "dense_map.h"

namespace aoc_2019_3 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::string_view> tmp;
    std::array<small_vector<Vec2i16, 512>, 2> moves;
    for (size_t i = 0; i < 2; i++) {
        for (std::string_view w : split(lines[i], tmp, ',')) {
            int n = 0;
            std::from_chars(w.data() + 1, w.data() + w.size(), n);
            ASSERT(n != 0);

            if (w[0] == 'U')
                moves[i].emplace_back(0, n);
            else if (w[0] == 'D')
                moves[i].emplace_back(0, -n);
            else if (w[0] == 'L')
                moves[i].emplace_back(-n, 0);
            else
                moves[i].emplace_back(n, 0);
        }
    }

    // Turn the two paths into rectilinear polylines, separated into horizontal
    // and vertical edges:
    std::array<small_vector<std::tuple<int, Vec2i64, Vec2i64>, 512>, 2> h_edges;
    std::array<small_vector<std::tuple<int, Vec2i64, Vec2i64>, 512>, 2> v_edges;
    std::array<small_vector<int, 512>, 2> steps_to_point;
    for (size_t i = 0; i < 2; i++) {
        Vec2i16 p{};
        steps_to_point[i].push_back(0);
        for (size_t j = 0; const Vec2i16 d : moves[i]) {
            auto q = p + d;
            auto &edges = d.x != 0 ? h_edges[i] : v_edges[i];
            edges.emplace_back(j, p.cast<int64_t>(), q.cast<int64_t>());
            steps_to_point[i].push_back(manhattan(d) + steps_to_point[i].back());
            p = q;
            ++j;
        }
    }

    // Find the intersection points between the two polylines. Since they are
    // rectilinear and parallel edges cannot intersect by definition, we need
    // only check horizontal against vertical edges for the two polygons.
    int min_dist = INT_MAX;
    int min_combined_steps = INT_MAX;
    for (size_t i = 0; i < 2; ++i) {
        for (const auto &[j, p1, p2] : h_edges[i]) {
            for (const auto &[k, q1, q2] : v_edges[1 - i]) {
                // Ignore the initial intersection at (0,0).
                if (j == 0 && k == 0) [[unlikely]]
                    continue;

                const int64_t dx = p1.x - p2.x;
                const int64_t dy = q1.y - q2.y;
                const int64_t cx = (q1.x * q2.y - q2.x * q1.y) * dx;
                const int64_t cy = (p1.x * p2.y - p2.x * p1.y) * dy;
                const Vec2i64 c = {-cx / (dx * dy), -cy / (dx * dy)};

                const bool intersect =
                    c.x >= std::min(p1.x, p2.x) && c.x <= std::max(p1.x, p2.x) &&
                    c.y >= std::min(q1.y, q2.y) && c.y <= std::max(q1.y, q2.y);

                if (intersect) {
                    min_dist = std::min<int64_t>(min_dist, manhattan(c));
                    const int steps = (steps_to_point[0][j] + std::abs(p1.x - c.x)) +
                                      (steps_to_point[1][k] + std::abs(q1.y - c.y));
                    min_combined_steps = std::min<int>(min_combined_steps, steps);
                }
            }
        }
    }

    fmt::print("{}\n", min_dist);
    fmt::print("{}\n", min_combined_steps);
}

}
