#include "common.h"
#include "dense_set.h"
#include "intcode.h"

namespace aoc_2019_15 {

using VM = IntcodeVM<SplitMemory<int16_t>>;

enum { N = 0, S = 1, W = 2, E = 3 };
enum { BLOCKED = 0, MOVED = 1, GOAL = 2 };

constexpr Vec2i8 dxdy[] = {Vec2i8(0, -1), Vec2i8(0, +1), Vec2i8(-1, 0), Vec2i8(+1, 0)};

static int turn(int dir, bool left)
{
    static constexpr uint8_t tab[] = {E, W, N, S};
    return tab[dir] ^ left;
}

void run(std::string_view buf)
{
    auto prog = find_numbers<VM::value_type>(buf);

    Matrix<char> m(100, 100);
    std::ranges::fill(m.all(), ' ');

    const Vec2u8 start(m.rows / 2, m.cols / 2);

    VM vm;
    vm.reset(prog);

    Vec2u8 p = start;
    auto step = [&](int dir) -> int {
        const Vec2u8 q = p + dxdy[dir].cast<uint8_t>();

        vm.run({static_cast<int16_t>(dir + 1)});
        const int ret = vm.output.front();
        vm.output.clear();
        m(q) = (ret == BLOCKED) ? '#' : '.';
        p = (ret == BLOCKED) ? p : q;
        return ret;
    };

    // Go north until we hit a wall.
    int dir = N;
    while (step(dir) == MOVED)
        ;

    // The classic maze algorithm: turn left if we managed to step forward,
    // otherwise turn right if there was something in front of us. This assumes
    // that the corridors are only one tile wide.
    dir = turn(dir, false);
    Vec2u8 goal{};
    do {
        if (int r = step(dir); r == GOAL)
            goal = p;
        else if (r == BLOCKED)
            dir = turn(dir, false);
        else
            dir = turn(dir, true);
    } while (p != start);

    // BFS from the goal.
    std::vector<std::pair<Vec2u8, int>> queue;
    queue.reserve(m.rows * m.cols / 2);
    queue.emplace_back(goal, 0);
    Matrix<bool> visited(m.rows, m.cols, false);
    int max_d = INT_MIN;
    for (size_t i = 0; i < queue.size(); i++) {
        auto [u, dxdy] = queue[i];
        visited(u) = true;
        max_d = std::max(max_d, dxdy);

        if (u == start)
            fmt::print("{}\n", dxdy);

        for (auto v : neighbors4(m, u))
            if ((m(v) != '#') && !visited(v))
                queue.emplace_back(v, dxdy + 1);
    }
    fmt::print("{}\n", max_d);
}

}
