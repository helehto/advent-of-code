#include "common.h"

namespace aoc_2024_15 {

constexpr auto move_to_dir = [] {
    std::array<std::pair<int8_t, int8_t>, 256> result{};
    result['^'] = std::pair(0, -1);
    result['<'] = std::pair(-1, 0);
    result['>'] = std::pair(+1, 0);
    result['v'] = std::pair(0, +1);
    return result;
}();

static Point<int> step1(Matrix<char> &grid, const Point<int> from, const char c)
{
    auto [dx, dy] = move_to_dir[c];
    const auto p = from.translate(dx, dy);

    if (grid(p) == '#')
        return from;

    if (grid(p) == '.') {
        grid(p) = '@';
        grid(from) = '.';
        return p;
    }

    ASSERT(grid(p) == 'O');
    auto q = p.translate(dx, dy);
    while (true) {
        if (grid(q) == '.') {
            grid(q) = 'O';
            grid(p) = '@';
            grid(from) = '.';
            return p;
        } else if (grid(q) == '#') {
            return from;
        } else {
            q = q.translate(dx, dy);
        }
    }
}

static int part1(const Matrix<char> &original_grid, std::string_view moves)
{
    auto grid = original_grid;

    Point<int> robot{};
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) == '@') {
            robot = p;
            break;
        }
    }

    for (char c : moves)
        robot = step1(grid, robot, c);

    int result = 0;
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) == 'O')
            result += 100 * p.y + p.x;
    }
    return result;
}

static Point<int> step2(const Point<int> robot,
                        Matrix<bool> &boxes,
                        const Matrix<bool> &walls,
                        const char c,
                        std::vector<Point<int>> &to_move)
{
    auto box_at = [&](const Point<int> &p) -> std::optional<Point<int>> {
        if (boxes(p))
            return p;
        if (auto q = p.translate(-1, 0); boxes(q))
            return q;
        return std::nullopt;
    };

    const auto [dx, dy] = move_to_dir[c];

    const auto target = robot.translate(dx, dy);
    if (walls(target))
        return robot;

    const auto first_box = box_at(target);
    if (!first_box.has_value())
        return target;

    to_move.clear();
    to_move.push_back(*first_box);

    if (dy != 0) {
        for (size_t i = 0; i < to_move.size(); ++i) {
            for (int k : {-1, 0, 1}) {
                auto b = to_move[i].translate(k, static_cast<int>(dy));
                if (boxes(b))
                    to_move.push_back(b);
            }
        }
    } else {
        auto step = 1 + (dx > 0);
        auto p = target;
        while (auto pp = box_at(p.translate(step * dx, step * dy))) {
            p = *pp;
            to_move.push_back(p);
        }
    }

    for (auto p : to_move) {
        auto q = p.translate(dx, dy);
        if (walls(q) || walls(q.translate(1, 0)))
            return robot;
    }

    for (auto p : to_move)
        boxes(p) = false;
    for (auto p : to_move)
        boxes(p.translate(dx, dy)) = true;
    return target;
}

static int part2(const Matrix<char> &grid, std::string_view moves)
{
    Point<int> robot{};
    Matrix<bool> boxes(grid.rows, 2 * grid.cols, false);
    Matrix<bool> walls(grid.rows, 2 * grid.cols, false);

    for (auto p : grid.ndindex<int>()) {
        const Point<int> pp(2 * p.x, p.y);
        if (grid(p) == '@') {
            robot = pp;
        } else if (grid(p) == 'O') {
            boxes(pp) = true;
        } else if (grid(p) == '#') {
            walls(pp) = true;
            walls(pp.translate(1, 0)) = true;
        }
    }

    std::vector<Point<int>> to_move;
    to_move.reserve(16);
    for (char c : moves)
        robot = step2(robot, boxes, walls, c, to_move);

    int result = 0;
    for (auto p : boxes.ndindex<int>())
        if (boxes(p))
            result += 100 * p.y + p.x;
    return result;
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto nl = std::ranges::find(lines, "") - lines.begin();
    auto grid = Matrix<char>::from_lines(std::span(lines.begin(), nl));

    std::string moves;
    for (std::string_view line : std::span(lines.begin() + nl + 1, lines.end()))
        moves.insert(moves.end(), line.begin(), line.end());

    fmt::print("{}\n", part1(grid, moves));
    fmt::print("{}\n", part2(grid, moves));
}

}
