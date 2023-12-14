#include "common.h"
#include <climits>
#include <set>

struct Sensor {
    Point<int> p;
    int d;
};

static std::vector<Point<int>> boundary(Sensor a, Sensor b)
{
    if (a.d >= b.d)
        std::swap(a, b);

    const auto [ax, ay] = a.p;
    const auto [bx, by] = b.p;
    const int dx = ax - bx;
    const int dy = ay - by;

    // The boundary finding code below was written with this assumption in
    // mind. It works on my input, and I can't be bothered to work out the
    // details of this corner case.
    ASSERT(dx != 0 && dy != 0);

    Point<int> b0, b1;

    if (dx > 0) {
        b0.x = std::max(bx, ax - (a.d + 1));
        b1.x = std::min(bx + (b.d + 1), ax);
    } else {
        b0.x = std::min(bx, ax + (a.d + 1));
        b1.x = std::max(bx - (b.d + 1), ax);
    }

    if (dy < 0) {
        b0.y = std::max(by - (b.d + 1), ay);
        b1.y = std::min(by, ay + (a.d + 1));
    } else {
        b0.y = std::min(by + (b.d + 1), ay);
        b1.y = std::max(by, ay - (a.d + 1));
    }

    const int sx = signum(dx);
    const int sy = signum(dy);

    std::vector<Point<int>> points;
    points.reserve(abs(b0.x - b1.x + 1) + abs(b0.y - b1.y + 1));
    for (int x = b0.x, y = b0.y; x != b1.x + sx; x += sx, y -= sy)
        points.push_back(Point{x, y});

    return points;
}

static Point<int> intersect(std::vector<Point<int>> &a, std::vector<Point<int>> &b)
{
    size_t ai = a.size() / 2;
    size_t bi = b.size() / 2;

    int i = 0;
    while (a[ai] != b[bi]) {
        if (i++ & 1) {
            int ml = manhattan(a[ai - 1], b[bi]);
            int mr = manhattan(a[ai + 1], b[bi]);
            ai = ml < mr ? ai - 1 : ai + 1;
        } else {
            int ml = manhattan(a[ai], b[bi - 1]);
            int mr = manhattan(a[ai], b[bi + 1]);
            bi = ml < mr ? bi - 1 : bi + 1;
        }
    }

    return a[ai];
}

void run_2022_15(FILE *f)
{
    std::vector<Sensor> sensors;
    std::string s;
    while (getline(f, s)) {
        auto v = find_numbers<int>(s);
        sensors.push_back(Sensor{
            .p = {v[0], v[1]},
            .d = abs(v[0] - v[2]) + abs(v[1] - v[3]),
        });
    }

    int target_y = sensors.size() > 20 ? 2000000 : 10;

    int xmin = INT_MAX;
    int xmax = INT_MIN;
    for (const auto &[p, d] : sensors) {
        const auto target_dy = abs(target_y - p.y);
        if (d >= target_dy) {
            const auto span = d - target_dy;
            xmin = std::min(xmin, p.x - span);
            xmax = std::max(xmax, p.x + span);
        }
    }
    fmt::print("{}\n", xmax - xmin);

    std::vector<std::vector<Point<int>>> bps;
    for (size_t i = 0; i < sensors.size(); i++) {
        for (size_t j = i + 1; j < sensors.size(); j++) {
            const auto &a = sensors[i];
            const auto &b = sensors[j];
            if (manhattan(a.p, b.p) == a.d + b.d + 2)
                bps.push_back(boundary(a, b));
        }
    }

    auto [x, y] = intersect(bps[0], bps[1]);
    fmt::print("{}\n", (x * UINT64_C(4'000'000) + y));
}
