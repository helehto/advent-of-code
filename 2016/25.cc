#include "common.h"

namespace aoc_2016_25 {

void run(std::string_view buf)
{
    // The assembly program adds the product of the two factors mentioned in
    // the first few instructions to the `a` register, and outputs the binary
    // representation of the resulting number in a loop, starting from the
    // lowest set bit. To get the desired output, `a` must be the difference
    // between this product and the next larger number of the form 0b101010...
    //
    // FIXME: The logic below might not work for all inputs.

    auto nums = find_numbers<int>(buf);
    auto factor = nums[0] * nums[1];

    int n = 0;
    while (n < factor)
        n = n << 2 | 0b10;

    fmt::print("{}\n", n - factor);
}

}
