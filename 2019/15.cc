#include "common.h"
#include "dense_set.h"
#include "intcode.h"

namespace aoc_2019_15 {

using VM = IntcodeVM<SplitMemory<int16_t>>;

enum { N = 0, S = 1, W = 2, E = 3 };
enum { BLOCKED = 0, MOVED = 1, GOAL = 2 };

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

    const Point<uint8_t> start(m.rows / 2, m.cols / 2);

    VM vm;
    vm.reset(prog);

    Point<uint8_t> p = start;
    auto step = [&](int dir) -> int {
        static constexpr int8_t dx[] = {0, 0, -1, +1};
        static constexpr int8_t dy[] = {-1, +1, 0, 0};
        const Point<uint8_t> q = p.translate(dx[dir], dy[dir]);

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
    Point<uint8_t> goal;
    do {
        if (int r = step(dir); r == GOAL)
            goal = p;
        else if (r == BLOCKED)
            dir = turn(dir, false);
        else
            dir = turn(dir, true);
    } while (p != start);

    // BFS from the goal.
    std::vector<std::pair<Point<uint8_t>, int>> queue;
    queue.reserve(m.rows * m.cols / 2);
    queue.emplace_back(goal, 0);
    Matrix<bool> visited(m.rows, m.cols, false);
    int max_d = INT_MIN;
    for (size_t i = 0; i < queue.size(); i++) {
        auto [u, d] = queue[i];
        visited(u) = true;
        max_d = std::max(max_d, d);

        if (u == start)
            fmt::print("{}\n", d);

        for (auto v : neighbors4(m, u))
            if ((m(v) != '#') && !visited(v))
                queue.emplace_back(v, d + 1);
    }
    fmt::print("{}\n", max_d);
}

}
