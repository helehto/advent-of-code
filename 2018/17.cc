#include "common.h"
#include "small_vector.h"

namespace aoc_2018_17 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto nums = find_numbers<int>(buf);

    int xmin = INT_MAX;
    int xmax = INT_MIN;
    int ymin = INT_MAX;
    int ymax = INT_MIN;

    for (size_t i = 0; std::string_view line : lines) {
        if (line[0] == 'x') {
            auto [x, y0, y1] = std::tuple(nums[i], nums[i + 1], nums[i + 2]);
            xmin = std::min(xmin, x);
            xmax = std::max(xmax, x);
            ymin = std::min(ymin, y0);
            ymax = std::max(ymax, y1);
        } else {
            auto [y, x0, x1] = std::tuple(nums[i], nums[i + 1], nums[i + 2]);
            xmin = std::min(xmin, x0);
            xmax = std::max(xmax, x1);
            ymin = std::min(ymin, y);
            ymax = std::max(ymax, y);
        }
        i += 3;
    }

    Matrix<char> grid(ymax - ymin + 1, xmax - xmin + 3, '.');

    for (size_t i = 0; std::string_view line : lines) {
        if (line[0] == 'x') {
            auto [x, y0, y1] = std::tuple(nums[i], nums[i + 1], nums[i + 2]);
            for (int y = y0; y <= y1; ++y)
                grid(y - ymin, x - xmin + 1) = '#';
        } else {
            auto [y, x0, x1] = std::tuple(nums[i], nums[i + 1], nums[i + 2]);
            for (int x = x0; x <= x1; ++x)
                grid(y - ymin, x - xmin + 1) = '#';
        }
        i += 3;
    }

    ASSERT(xmin <= 500 && 500 <= xmax);

    constexpr auto below = [](Vec2i v) { return v + Vec2i{0, 1}; };
    constexpr auto left = [](Vec2i v) { return v + Vec2i{-1, 0}; };
    constexpr auto right = [](Vec2i v) { return v + Vec2i{1, 0}; };

    const auto can_fall = [&](Vec2i v) {
        return grid.in_bounds(below(v)) &&
               (grid(below(v)) == '.' || grid(below(v)) == '|');
    };

    small_vector<Vec2i, 64> sources;
    sources.push_back({500 - xmin + 1, std::max(0, -ymin)});

    size_t i = 0;
    for (; !sources.empty(); ++i) {
        Vec2i p = sources.back();
        sources.pop_back();

        // If this tile was previously marked as a source, but is now water at
        // rest, some other source happened to get here before this one did --
        // discard it.
        if (grid(p) == '~')
            continue;

        // Traverse downwards from this source, marking water as flowing along
        // the way.
        for (; can_fall(p); p = below(p))
            grid(p) = '|';
        if (!grid.in_bounds(below(p))) {
            grid(p) = '|';
            continue;
        }

        // We have either reached some clay or water at rest. Scan leftwards
        // and rightwards to try to find an edge from which water can fall.
        Vec2i l = p;
        while (!can_fall(l) && grid(left(l)) != '#')
            l.x--;
        Vec2i r = p;
        while (!can_fall(r) && grid(right(r)) != '#')
            r.x++;

        if (!can_fall(l) && !can_fall(r)) {
            // If we did not find any edge, water cannot fall from here, so it
            // is at rest. Fill the current level and move the source upwards
            // one tile to (eventually) completely fill the container.
            std::fill(&grid(l), &grid(r) + 1, '~');
            sources.push_back(p + Vec2i{0, -1});
        } else {
            // On the other hand, if there _is_ an edge on this level, the
            // water must be flowing. Mark it as such and add the edge(s) as
            // new sources.
            std::fill(&grid(l), &grid(r) + 1, '|');
            if (grid(below(l)) == '.')
                sources.push_back(l);
            if (grid(below(r)) == '.')
                sources.push_back(r);
        }
    }

    const size_t at_rest = std::ranges::count(grid.all(), '~');
    const size_t flowing = std::ranges::count(grid.all(), '|');
    fmt::print("{}\n{}\n", at_rest + flowing, at_rest);
}

}
