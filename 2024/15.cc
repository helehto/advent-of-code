#include <algorithm>
#include <aoc/base.h>
#include <aoc/inplace_vector.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace aoc_2024_15 {

constexpr auto move_to_dir = [] {
    std::array<Vec2i8, 256> result{};
    result['^'] = Vec2i8(0, -1);
    result['<'] = Vec2i8(-1, 0);
    result['>'] = Vec2i8(+1, 0);
    result['v'] = Vec2i8(0, +1);
    return result;
}();

static Vec2i step1(MatrixView<char> grid, const Vec2i from, const char c)
{
    const auto d = move_to_dir[c];
    const auto p = from + d;

    if (grid(p) == '#')
        return from;

    if (grid(p) == '.') {
        grid(p) = '@';
        grid(from) = '.';
        return p;
    }

    ASSERT(grid(p) == 'O');
    auto q = p + d;
    while (true) {
        if (grid(q) == '.') {
            grid(q) = 'O';
            grid(p) = '@';
            grid(from) = '.';
            return p;
        } else if (grid(q) == '#') {
            return from;
        } else {
            q = q + d;
        }
    }
}

static int part1(const Matrix<char> &original_grid, std::string_view moves)
{
    Matrix<char> grid = original_grid;

    Vec2i robot = [&] {
        for (size_t i = 0; i < grid.rows; ++i)
            for (size_t j = 0; j < grid.cols; ++j)
                if (grid(i, j) == '@')
                    return Vec2i(j, i);
        ASSERT_MSG(false, "No robot found");
    }();

    for (char c : moves)
        robot = step1(grid, robot, c);

    int result = 0;
    for (size_t i = 0; i < grid.rows; ++i)
        for (size_t j = 0; j < grid.cols; ++j)
            result += grid(i, j) == 'O' ? 100 * i + j : 0;

    return result;
}

static Vec2i step2(const Vec2i robot,
                   Matrix<bool> &boxes,
                   MatrixView<const bool> walls,
                   const char c,
                   inplace_vector<Vec2i, 64> &to_move)
{
    auto box_at = [&](const Vec2i &p) -> std::optional<Vec2i> {
        if (boxes(p))
            return p;
        if (auto q = p + Vec2i(-1, 0); boxes(q))
            return q;
        return std::nullopt;
    };

    const auto d = move_to_dir[c];
    const auto target = robot + d;
    if (walls(target))
        return robot;

    const auto first_box = box_at(target);
    if (!first_box.has_value())
        return target;

    to_move.clear();
    to_move.unchecked_push_back(*first_box);

    if (d.y != 0) {
        for (size_t i = 0; i < to_move.size(); ++i) {
            for (int k : {-1, 0, 1}) {
                auto b = to_move[i] + Vec2i(k, d.y);
                if (boxes(b))
                    to_move.unchecked_push_back(b);
            }
        }
    } else {
        auto step = 1 + (d.x > 0);
        auto p = target;
        while (auto pp = box_at(p + step * d)) {
            p = *pp;
            to_move.unchecked_push_back(p);
        }
    }

    for (auto p : to_move) {
        auto q = p + d;
        if (walls(q) || walls(q + Vec2i(1, 0)))
            return robot;
    }

    for (auto p : to_move)
        boxes(p) = false;
    for (auto p : to_move)
        boxes(p + d) = true;
    return target;
}

static int part2(MatrixView<const char> grid, std::string_view moves)
{
    Vec2i robot{};
    Matrix<bool> boxes(grid.rows, 2 * grid.cols, false);
    Matrix<bool> walls(grid.rows, 2 * grid.cols, false);

    for (size_t i = 0; i < grid.rows; ++i) {
        for (size_t j = 0; j < grid.cols; ++j) {
            const Vec2i pp(2 * j, i);
            if (grid(i, j) == '@') {
                robot = pp;
            } else if (grid(i, j) == 'O') {
                boxes(pp) = true;
            } else if (grid(i, j) == '#') {
                walls(pp) = true;
                walls(pp + Vec2i(1, 0)) = true;
            }
        }
    }

    inplace_vector<Vec2i, 64> to_move;
    for (char c : moves)
        robot = step2(robot, boxes, walls, c, to_move);

    int result = 0;
    for (size_t i = 0; i < boxes.rows; ++i)
        for (size_t j = 0; j < boxes.cols; ++j)
            result += boxes(i, j) ? 100 * i + j : 0;

    return result;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    auto nl = std::ranges::find(lines, std::string_view("")) - lines.begin();
    auto grid = Matrix<char>::from_lines(std::span(lines.begin(), nl));

    std::string moves;
    for (std::string_view line : std::span(lines.begin() + nl + 1, lines.end()))
        moves.insert(moves.end(), line.begin(), line.end());

    answer.add(part1(grid, moves));
    answer.add(part2(grid, moves));
}
AOC_REGISTER_SOLVER(2024, 15, run);

}
