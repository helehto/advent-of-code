#include "common.h"
#include "dense_set.h"
#include "intcode.h"

namespace aoc_2019_11 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(std::string_view buf)
{
    const auto prog = find_numbers<VM::value_type>(buf);

    dense_set<Vec2i16> white_panels;
    white_panels.reserve(2000);
    dense_set<Vec2i16> visited;
    visited.reserve(2000);

    auto paint = [&](Vec2i16 p, bool white) {
        if (white)
            white_panels.emplace(p);
        else
            white_panels.erase(p);
    };

    Vec2i16 d(0, -1);
    Vec2i16 p{};
    VM vm;
    vm.reset(prog);
    while (vm.run({static_cast<int>(white_panels.count(p))}) != HaltReason::op99) {
        paint(p, vm.output[0] != 0);
        visited.insert(p);
        d = vm.output[1] ? d.cw() : d.ccw();
        p += d;
        vm.output.clear();
    }
    fmt::print("{}\n", visited.size());

    vm.reset(prog);
    white_panels.clear();
    p = {};
    white_panels.insert(p);
    while (vm.run({static_cast<int>(white_panels.count(p))}) != HaltReason::op99) {
        paint(p, vm.output[0] != 0);
        d = vm.output[1] ? d.cw() : d.ccw();
        p += d;
        vm.output.clear();
    }

    int16_t min_x = INT16_MAX;
    int16_t min_y = INT16_MAX;
    int16_t max_x = INT16_MIN;
    int16_t max_y = INT16_MIN;
    for (const Vec2i16 &p : white_panels) {
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++)
            fmt::print("{}", white_panels.count(Vec2i16(x, y)) ? '#' : ' ');
        fmt::print("\n");
    }
}

}
