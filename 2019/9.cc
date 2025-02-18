#include "common.h"
#include "intcode.h"

namespace aoc_2019_9 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(std::string_view buf)
{
    const auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);
    vm.run({1});
    fmt::print("{}\n", vm.output.front());

    vm.reset(prog);
    vm.run({2});
    fmt::print("{}\n", vm.output.front());
}

}
