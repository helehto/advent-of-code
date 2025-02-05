#include "common.h"
#include "intcode.h"

namespace aoc_2019_25 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(FILE *f)
{
    auto buf = slurp(f);
    auto prog = find_numbers<VM::value_type>(buf);

    VM vm;
    vm.reset(prog);

    // TODO: No hard-coding...
    std::string_view moves[] = {
        "north\n", "north\n", "take monolith\n", "north\n", "take hypercube\n",
        "south\n", "south\n", "east\n",          "east\n",  "take easter egg\n",
        "east\n",  "south\n", "take ornament\n", "west\n",  "south\n",
        "west\n",
    };

    for (std::string_view move : moves) {
        vm.output.clear();
        for (char c : move)
            vm.input.push_back(c);
        vm.run();
    }

    std::vector<int> nums;
    std::string s;
    for (char c : vm.output)
        s.push_back(c);
    vm.output.clear();
    find_numbers(s, nums);
    fmt::print("{}\n", nums[0]);
}

}
