#include "common.h"
#include "dense_map.h"

namespace aoc_2019_10 {

static double to_angle(Point<int16_t> a)
{
    double angle = -std::atan2<double>(-a.y, a.x) * (180.0 / M_PI) + 90.0;

    if (angle < 0.0)
        angle += 360.0;

    return std::fabs(angle);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<Point<int16_t>> asteroids;
    for (size_t y = 0; y < lines.size(); y++) {
        const char *line = lines[y].data();
        for (size_t x = 0; x < lines[y].size(); x++)
            if (line[x] != '.')
                asteroids.emplace_back(x, y);
    }

    Point<int16_t> station;
    dense_map<Point<int16_t>, std::vector<Point<int16_t>>> angles, best_angles;
    for (size_t i = 0; i < asteroids.size(); ++i) {
        angles.clear();

        for (size_t j = 0; j < asteroids.size(); ++j) {
            if (i == j)
                continue;

            const int16_t dx = asteroids[j].x - asteroids[i].x;
            const int16_t dy = asteroids[j].y - asteroids[i].y;
            const auto gcd = std::gcd(dx, dy);
            const int16_t dx_reduced = gcd > 0 ? dx / gcd : dx;
            const int16_t dy_reduced = gcd > 0 ? dy / gcd : dy;
            angles[{dx_reduced, dy_reduced}].push_back(asteroids[j]);
        }

        if (best_angles.size() < angles.size()) {
            best_angles = std::move(angles);
            station = asteroids[i];
        }
    }
    fmt::print("{}\n", best_angles.size());

    std::vector<std::pair<Point<int16_t>, std::vector<Point<int16_t>>>>
        targets_by_direction(best_angles.begin(), best_angles.end());

    std::ranges::sort(targets_by_direction, {},
                      [](auto &p) { return to_angle(p.first); });
    for (auto &[_, points] : targets_by_direction) {
        std::ranges::sort(points, std::greater<>(),
                          [&](const auto &p) { return manhattan(station, p); });
    }

    int num_vaporized = 0;
    while (!targets_by_direction.empty()) {
        for (auto it = targets_by_direction.begin(); it != targets_by_direction.end();) {
            auto &targets = it->second;
            ASSERT(!targets.empty());
            num_vaporized++;
            if (num_vaporized == 200) {
                const auto [x, y] = targets.back();
                fmt::print("{}\n", 100 * x + y);
                return;
            }
            targets.pop_back();
            if (targets.empty())
                it = targets_by_direction.erase(it);
            else
                ++it;
        }
    }
}

}
