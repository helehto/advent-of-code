#include "common.h"

namespace aoc_2020_23 {

void run(std::string_view buf)
{
    auto storage = std::make_unique_for_overwrite<std::byte[]>(1024 * 1024 * 1024);

    std::span<int> v(std::bit_cast<int *>(storage.get()), 10);
    v[0] = -99;
    for (size_t i = 1; i < buf.size(); ++i)
        v[buf[i - 1] - '0'] = buf[i] - '0';
    v[buf.back() - '0'] = buf[0] - '0';

    auto arrange = [&](int iterations, int curr) {
        for (int i = 1; i <= iterations; ++i) {
            const auto a = v[curr];
            const auto b = v[a];
            const auto c = v[b];
            v[curr] = v[c];

            int dest = curr > 1 ? curr - 1 : v.size() - 1;
            while (dest == a || dest == b || dest == c)
                dest = dest > 1 ? dest - 1 : v.size() - 1;

            v[c] = v[dest];
            v[dest] = a;
            curr = v[curr];
        }
    };

    // Part 1:
    arrange(100, buf[0] - '0');
    for (int d = v[1]; d != 1; d = v[d])
        putc(d + '0', stdout);
    putc('\n', stdout);

    // Part 2:
    {
        v = std::span(std::bit_cast<int *>(storage.get()), 1'000'001);
        for (size_t i = 1; i < buf.size(); ++i)
            v[buf[i - 1] - '0'] = buf[i] - '0';
        v[buf.back() - '0'] = 10;
        for (size_t i = 10; i <= 1'000'000; ++i)
            v[i] = i + 1;
        v.back() = buf[0] - '0';

        arrange(10'000'000, buf[0] - '0');
        fmt::print("{}\n", static_cast<uint64_t>(v[1]) * v[v[1]]);
    }
}

}
