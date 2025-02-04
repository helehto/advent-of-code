#include "common.h"
#include "intcode.h"

namespace aoc_2019_23 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

void run(FILE *f)
{
    auto buf = slurp(f);
    auto prog = find_numbers<VM::value_type>(buf);

    VM computers[50];
    for (size_t i = 0; i < std::size(computers); ++i) {
        computers[i].reset(prog);
        computers[i].input.push_back(i);
        computers[i].input.push_back(-1);
    }

    std::optional<VM::value_type> first_y_to_nat;
    std::optional<VM::value_type> last_y_from_nat;
    VM::value_type nat_x = 0;
    VM::value_type nat_y = 0;
    while (true) {
        bool network_is_idle = true;
        for (size_t i = 0; i < std::size(computers); ++i) {
            if (computers[i].input.empty())
                continue;

            computers[i].run();
            network_is_idle &= computers[i].output.empty();

            for (size_t j = 0; j < computers[i].output.size(); j += 3) {
                const auto address = computers[i].output[j + 0];
                const auto x = computers[i].output[j + 1];
                const auto y = computers[i].output[j + 2];

                if (address == 255) {
                    if (!first_y_to_nat.has_value())
                        first_y_to_nat = y;
                    nat_x = x;
                    nat_y = y;
                    continue;
                }

                computers[address].input.push_back(x);
                computers[address].input.push_back(y);
            }
            computers[i].output.clear();
        }

        if (network_is_idle) {
            if (last_y_from_nat && nat_y == *last_y_from_nat)
                break;
            computers[0].input.push_back(nat_x);
            computers[0].input.push_back(nat_y);
            last_y_from_nat = nat_y;
        }
    }

    fmt::print("{}\n", *first_y_to_nat);
    fmt::print("{}\n", *last_y_from_nat);
}

}
