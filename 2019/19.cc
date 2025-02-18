#include "common.h"
#include "intcode.h"

namespace aoc_2019_19 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(std::string_view buf)
{
    auto prog = find_numbers<VM::value_type>(buf);
    VM vm;

    auto scan = [&](const Vec2i p) {
        vm.reset(prog);
        ASSERT(vm.run({p.x, p.y}) == HaltReason::op99);
        ASSERT(vm.output.size() == 1);
        return vm.output[0] != 0;
    };

    auto fits = [&](Vec2i p) {
        return scan(p.translate(0, 0)) && scan(p.translate(0, 99)) &&
               scan(p.translate(99, 0)) && scan(p.translate(99, 99));
    };

    Matrix<bool> g(50, 50);
    for (int x = 0; x < 50; ++x) {
        for (int y = 0; y < 50; ++y) {
            Vec2i p{x, y};
            g(p) = scan(p);
        }
    }

    int n = 0;
    for (int x = 0; x < 50; ++x)
        for (int y = 0; y < 50; ++y)
            n += g(x, y);

    Vec2i p(25, 0);
    for (; !g(p); p.y++)
        ;

    for (;; p = p.translate(1, 0)) {
        while (!scan(p))
            p = p.translate(0, 1);
        if (p.x >= 99 && p.y >= 99 && fits(p.translate(-99, 0)))
            break;
    }

    p = p.translate(-99, 0);
    while (true) {
        if (Vec2i q = p.translate(-1, 0); fits(q))
            p = q;
        else if (Vec2i q = p.translate(0, -1); fits(q))
            p = q;
        else
            break;
    }
    fmt::print("{}\n{}\n", n, 10000 * p.x + p.y);
}

}
