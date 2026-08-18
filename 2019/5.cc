#include "intcode.h"
#include <aoc/base.h>
#include <aoc/string.h>
#include <string_view>

namespace aoc_2019_5 {

using VM = IntcodeVM<FlatMemory<int>>;

void run(std::string_view buf, aoc::Answer &answer)
{
    auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);
    vm.run({1});
    answer.add(vm.output.back());

    vm.reset(prog);
    vm.run({5});
    answer.add(vm.output.back());
}
AOC_REGISTER_SOLVER(2019, 5, run);

}
