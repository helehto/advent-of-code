#include "intcode.h"
#include <aoc/base.h>
#include <aoc/dense_set.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace aoc_2019_13 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(std::string_view buf, aoc::Answer &answer)
{
    auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);
    vm.run();

    dense_set<Vec2i> block_tiles;
    ASSERT(vm.output.size() % 3 == 0);
    for (size_t i = 0; i < vm.output.size(); i += 3) {
        const int x = vm.output[i];
        const int y = vm.output[i + 1];
        const int id = vm.output[i + 2];
        if (id == 2)
            block_tiles.insert({x, y});
    }
    answer.add(block_tiles.size());

    prog[0] = 2;
    vm.reset(prog);
    int paddle_x = -1;
    int ball_x = -1;
    int score = -1;
    HaltReason reason;
    do {
        vm.output.clear();

        // Steer the paddle towards the ball for each input.
        reason = vm.run({signum(ball_x - paddle_x)});

        ASSERT(vm.output.size() % 3 == 0);
        for (size_t i = 0; i < vm.output.size(); i += 3) {
            const int x = vm.output[i];
            const int y = vm.output[i + 1];
            const int id = vm.output[i + 2];

            if (x == -1 && y == 0)
                score = id;
            else if (id == 3)
                paddle_x = x;
            else if (id == 4)
                ball_x = x;
        }
    } while (reason != HaltReason::op99);
    answer.add(score);
}
AOC_REGISTER_SOLVER(2019, 13, run);

}
