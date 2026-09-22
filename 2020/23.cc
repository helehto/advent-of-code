#include <aoc/base.h>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace aoc_2020_23 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto storage = std::make_unique_for_overwrite<std::byte[]>(1024 * 1024 * 1024);

    std::span<uint32_t> v(std::bit_cast<uint32_t *>(storage.get()), 10);
    for (size_t i = 1; i < buf.size(); ++i)
        v[buf[i - 1] - '0'] = buf[i] - '0';
    v[buf.back() - '0'] = buf[0] - '0';

    auto arrange = [&](int iterations, uint32_t curr) {
        const auto n = static_cast<uint32_t>(v.size());
        for (int i = 1; i <= iterations; ++i) {
            const auto a = v[curr];
            const auto b = v[a];
            const auto c = v[b];
            v[curr] = v[c];

            uint32_t dest = curr > 1 ? curr - 1 : n - 1;
            while (dest == a || dest == b || dest == c)
                dest = dest > 1 ? dest - 1 : n - 1;

            v[c] = v[dest];
            v[dest] = a;
            curr = v[curr];
        }
    };

    // Part 1:
    arrange(100, buf[0] - '0');
    std::string part1;
    for (uint32_t d = v[1]; d != 1; d = v[d])
        part1 += static_cast<char>(d + '0');
    answer.add(part1);

    // Part 2:
    {
        v = std::span(std::bit_cast<uint32_t *>(storage.get()), 1'000'001);
        for (size_t i = 1; i < buf.size(); ++i)
            v[buf[i - 1] - '0'] = buf[i] - '0';
        v[buf.back() - '0'] = 10;
        for (size_t i = 10; i <= 1'000'000; ++i)
            v[i] = i + 1;
        v.back() = buf[0] - '0';

        arrange(10'000'000, buf[0] - '0');
        answer.add(static_cast<uint64_t>(v[1]) * v[v[1]]);
    }
}
AOC_REGISTER_SOLVER(2020, 23, run);

}
