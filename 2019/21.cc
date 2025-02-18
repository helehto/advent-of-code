#include "common.h"
#include "intcode.h"

namespace aoc_2019_21 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(std::string_view buf)
{
    auto prog = find_numbers<VM::value_type>(buf);

    static constexpr char part1[] = "NOT A T\n"
                                    "NOT T T\n"
                                    "AND B T\n"
                                    "AND C T\n"
                                    "NOT T J\n"
                                    "AND D J\n"
                                    "WALK\n";

    static constexpr char part2[] = "NOT A T\n"
                                    "NOT T T\n"
                                    "AND B T\n"
                                    "AND C T\n"
                                    "NOT T J\n"
                                    "AND D J\n"
                                    "NOT H T\n"
                                    "NOT T T\n"
                                    "OR E T\n"
                                    "AND T J\n"
                                    "RUN\n";

    VM vm;
    vm.reset(prog);
    for (char c : part1)
        vm.input.push_back(c);
    vm.run();
    fmt::print("{}\n", vm.output.back());

    vm.reset(prog);
    for (char c : part2)
        vm.input.push_back(c);
    vm.run();
    fmt::print("{}\n", vm.output.back());
}

}
