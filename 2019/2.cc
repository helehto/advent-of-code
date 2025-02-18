#include "common.h"
#include "intcode.h"

namespace aoc_2019_2 {

using VM = IntcodeVM<FlatMemory<int>>;

void run(std::string_view buf)
{
    auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);
    vm.mem.wr(1, 12);
    vm.mem.wr(2, 5);
    vm.run();
    fmt::print("{}\n", vm.mem.rd(0));

    for (int verb = 1; verb < 100; ++verb) {
        for (int noun = 1; noun < 100; ++noun) {
            vm.reset(prog);
            vm.mem.wr(1, noun);
            vm.mem.wr(2, verb);
            vm.run();
            if (vm.mem.rd(0) == 19690720) {
                fmt::print("{}\n", 100 * noun + verb);
                break;
            }
        }
    }
}
}
