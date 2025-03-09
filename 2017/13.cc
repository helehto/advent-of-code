#include "common.h"

namespace aoc_2017_13 {

void run(std::string_view buf)
{
    struct Scanner {
        uint8_t depth;
        uint8_t range;
    };

    auto nums = find_numbers<uint8_t>(buf);
    std::span scanners(reinterpret_cast<Scanner *>(nums.data()), nums.size() / 2);

    auto severity = [&](int wait) -> std::optional<int> {
        int result = 0;
        bool caught = false;

        for (auto &[depth, range] : scanners) {
            auto at = (wait + depth) % (2 * range - 2);
            if (at >= range)
                at = ((2 * range - 2) - at) % range;
            if (at == 0) {
                caught = true;
                result += depth * range;
            }
        }

        if (caught)
            return result;

        return std::nullopt;
    };

    fmt::print("{}\n", *severity(0));

    int period = 1;
    for (auto &[_, range] : scanners)
        period = std::lcm(period, 2 * range - 2);

    for (int t = 0; t < 2 * period; ++t) {
        if (auto sev = severity(t); !sev) {
            fmt::print("{}\n", t);
            break;
        }
    }
}

}
