#include "common.h"
#include "intcode.h"

namespace aoc_2019_5 {

using VM = IntcodeVM<FlatMemory<int>>;

void run(FILE *f)
{
    auto buf = slurp(f);
    auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);
    vm.run({1});
    fmt::print("{}\n", vm.output.back());

    vm.reset(prog);
    vm.run({5});
    fmt::print("{}\n", vm.output.back());
}

}
