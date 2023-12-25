#include "common.h"

namespace aoc_2022_22 {

enum { U, L, D, R };

static constexpr std::pair<int, int> face_orientations[6][4] = {
    {{5, L}, {3, L}, {2, U}, {1, L}}, {{5, D}, {0, R}, {2, R}, {4, R}},
    {{0, D}, {3, U}, {4, U}, {1, D}}, {{2, L}, {0, L}, {5, U}, {4, L}},
    {{2, D}, {3, R}, {5, R}, {1, R}}, {{3, D}, {0, U}, {1, U}, {4, D}},
};

static Point<int> turn_left(Point<int> d)
{
    return {-d.y, d.x};
}

static Point<int> turn_right(Point<int> d)
{
    return {d.y, -d.x};
}

struct Move {
    enum { L, R, N };
    uint16_t n : 14;
    uint16_t type : 2;
};

static Point<int> move1(const std::vector<std::string> &map, Point<int> p, Point<int> d)
{
    for (;;) {
        p.x = (p.x + d.x) % (int)map.size();
        p.y = (p.y + d.y) % (int)map[0].size();
        if (p.x < 0)
            p.x += map.size();
        if (p.y < 0)
            p.y += map[0].size();
        if (map[p.x][p.y] != ' ')
            return p;
    }
}

static int score(Point<int> p, Point<int> d)
{
    const auto f = (d.x == 0 && d.y == 1)    ? 0
                   : (d.x == 1 && d.y == 0)  ? 1
                   : (d.x == 0 && d.y == -1) ? 2
                                             : 3;
    return 1000 * (p.x + 1) + 4 * (p.y + 1) + f;
}

static int part1(const std::vector<std::string> &map, const std::vector<Move> &moves)
{
    Point<int> d{0, 1};
    auto p = move1(map, {0, -1}, d);

    for (const auto &move : moves) {
        switch (move.type) {
        case Move::L:
            d = turn_left(d);
            break;
        case Move::R:
            d = turn_right(d);
            break;
        case Move::N:
            for (size_t i = 0; i < move.n; i++) {
                const auto npos = move1(map, p, d);
                if (map[npos.x][npos.y] == '#')
                    break;
                p = npos;
            }
        }
    }

    return score(p, d);
}

static int part2(const std::vector<std::string> &map, const std::vector<Move> &moves)
{
    constexpr int face_width = 50;
    std::vector<Point<int>> face_to_global;

    auto wrap = [&](Point<int> localp, int face,
                    Point<int> d) -> std::tuple<Point<int>, int, Point<int>> {
        auto [y, x] = localp;
        int exit_dir, k;

        if (y < 0 || y >= face_width) {
            exit_dir = y < 0 ? U : D;
            k = x;
        } else if (x < 0 || x >= face_width) {
            exit_dir = x < 0 ? L : R;
            k = y;
        } else {
            return {localp, face, d};
        }

        auto [nface, entry_dir] = face_orientations[face][exit_dir];
        switch (exit_dir << 2 | entry_dir) {
        case L << 2 | L:
        case R << 2 | R:
        case U << 2 | U:
        case D << 2 | D:
        case R << 2 | U:
        case U << 2 | R:
        case L << 2 | D:
        case D << 2 | L:
            k = face_width - 1 - k;
        }

        switch (entry_dir) {
        case U:
            return {{0, k}, nface, {1, 0}};
        case L:
            return {{k, 0}, nface, {0, 1}};
        case D:
            return {{face_width - 1, k}, nface, {-1, 0}};
        default:
            return {{k, face_width - 1}, nface, {0, -1}};
        }
    };

    int face = 0;
    for (size_t i = 0; i < map.size(); i += face_width) {
        for (size_t j = 0; j < map[0].size(); j += face_width) {
            if (map[i][j] != ' ') {
                face_to_global.emplace_back(i, j);
                face++;
            }
        }
    }

    face = 0;
    Point<int> local{0, 0};
    Point<int> d{0, 1};

    for (const auto &move : moves) {
        switch (move.type) {
        case Move::L:
            d = turn_left(d);
            break;
        case Move::R:
            d = turn_right(d);
            break;
        case Move::N:
            for (size_t i = 0; i < move.n; i++) {
                auto [nlocal, nface, nd] = wrap(local.translate(d.x, d.y), face, d);
                if (map[face_to_global[nface].x + nlocal.x]
                       [face_to_global[nface].y + nlocal.y] == '#')
                    break;
                local = nlocal;
                face = nface;
                d = nd;
            }
        }
    }

    auto x = face_to_global[face].x + local.x;
    auto y = face_to_global[face].y + local.y;
    return score({x, y}, d);
}

static std::vector<Move> parse_moves(const std::string_view &s)
{
    size_t i = 0;
    std::vector<Move> moves;

    while (i < s.size()) {
        if (s[i] == 'L') {
            moves.push_back(Move{.n = 0, .type = Move::L});
            i++;
        } else if (s[i] == 'R') {
            moves.push_back(Move{.n = 0, .type = Move::R});
            i++;
        } else {
            uint16_t n = 0;
            ASSERT(isdigit(s[i]));
            do {
                n = 10 * n + s[i] - '0';
                i++;
            } while (i < s.size() && isdigit(s[i]));
            moves.push_back(Move{.n = n, .type = Move::N});
        }
    }

    return moves;
}

void run(FILE *f)
{
    std::vector<std::string> map;
    std::string s;
    while (getline(f, s) && !s.empty())
        map.push_back(s);

    size_t m = 0;
    for (auto &line : map)
        m = std::max(m, line.size());
    for (auto &line : map) {
        if (line.size() != m)
            line.resize(m, ' ');
    }

    getline(f, s);
    auto moves = parse_moves(s);

    fmt::print("{}\n", part1(map, moves));
    fmt::print("{}\n", part2(map, moves));
}

}
