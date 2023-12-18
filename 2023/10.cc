#include "common.h"
#include "dense_set.h"
#include <queue>

static std::array<Point<size_t>, 2> get_pipe_neighbors(Point<size_t> p, char c)
{
    switch (c) {
    case '|':
        return {{p.translate(0, -1), p.translate(0, +1)}};
    case '-':
        return {{p.translate(-1, 0), p.translate(+1, 0)}};
    case 'L':
        return {{p.translate(0, -1), p.translate(+1, 0)}};
    case 'J':
        return {{p.translate(0, -1), p.translate(-1, 0)}};
    case '7':
        return {{p.translate(-1, 0), p.translate(0, +1)}};
    case 'F':
        return {{p.translate(+1, 0), p.translate(0, +1)}};
    }
    ASSERT(false);
}

static int64_t signed_area(std::span<const Point<size_t>> points)
{
    ASSERT(points.front() == points.back());

    int64_t result = 0;
    size_t i = 1;
    for (; i < points.size(); i++) {
        Point<size_t> p0 = points[i - 1];
        Point<size_t> p1 = points[i];
        result += (int64_t)(p0.x * p1.y) - (int64_t)(p1.x * p0.y);
    }

    return result;
}

void run_2023_10(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);

    Point<size_t> start{};
    for (auto p : grid.ndindex())
        if (grid(p) == 'S')
            start = p;

    // Find the two pipes connected to the start point.
    std::array<Point<size_t>, 2> start_neighbors;
    {
        size_t i = 0;
        if (auto q = start.translate(0, -1); strchr("|F7", grid(q)))
            start_neighbors[i++] = q;
        if (auto q = start.translate(-1, 0); strchr("-LF", grid(q)))
            start_neighbors[i++] = q;
        if (auto q = start.translate(+1, 0); strchr("-J7", grid(q)))
            start_neighbors[i++] = q;
        if (auto q = start.translate(0, +1); strchr("|JL", grid(q)))
            start_neighbors[i++] = q;
        ASSERT(i == 2);
    }

    bool start_is_corner = (start_neighbors[0].x != start_neighbors[1].x &&
                            start_neighbors[0].y != start_neighbors[1].y);

    std::vector<Point<size_t>> path{start};
    std::vector<Point<size_t>> polygon;
    if (start_is_corner)
        polygon.push_back(start);

    for (Point<size_t> prev = start, curr = start_neighbors[0]; curr != start;) {
        auto n = get_pipe_neighbors(curr, grid(curr));
        path.push_back(curr);
        if (strchr("F7JL", grid(curr)))
            polygon.push_back(curr);
        prev = std::exchange(curr, n[0] != prev ? n[0] : n[1]);
    }
    path.push_back(path.front());
    polygon.push_back(polygon.front());
    fmt::print("{}\n", path.size() / 2);

    // Remove any superfluous pipes from the grid.
    dense_set<Point<size_t>> path_set(path.begin(), path.end());
    for (auto p : grid.ndindex()) {
        if (!path_set.count(p))
            grid(p) = '.';
    }

    // Make sure the polygon is oriented correctly.
    if (signed_area(polygon) < 0) {
        std::reverse(polygon.begin(), polygon.end());
        std::reverse(path.begin(), path.end());
    }

    std::queue<Point<size_t>> queue;
    int area = 0;
    auto fill = [&](Point<size_t> q) {
        if (grid.in_bounds(q) && grid(q) == '.') {
            grid(q) = 'X';
            queue.push(q);
            area++;
        }
    };

    // Mark any points to the side of the polygon edges as filled.
    for (size_t i = 1; i < path.size(); i++) {
        const Point<size_t> p0 = path[i - 1];
        const Point<size_t> p1 = path[i];
        const auto dx = int64_t(p1.x) - int64_t(p0.x);
        const auto dy = int64_t(p1.y) - int64_t(p0.y);

        fill(p1.translate(-dy, dx));

        // Corners have two possibly adjacent squares inside the polygon; check
        // the other one as well.
        if (grid(p1) == 'F')
            fill(p1.translate(dx, -dy));
    }

    // Flood fill from the initially marked points near the edges to count the
    // entire interior of the polygon.
    while (!queue.empty()) {
        Point p = queue.front();
        queue.pop();
        for (Point q : neighbors4(grid, p))
            fill(q);
    }
    fmt::print("{}\n", area);
}
