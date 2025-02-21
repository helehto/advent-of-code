#include "common.h"
#include "md5.h"

namespace aoc_2016_5 {

static std::pair<uint32_t, uint32_t> solve(std::string_view prefix)
{
    md5::State md5(prefix);

    int characters = 0;
    uint32_t password1 = 0;
    uint32_t password2 = 0;
    int password2_mask = 0;

    for (int n = 0;; n += 8) {
        const __m256i hashes = md5.run(n).a;
        const uint32_t mask5 = md5::leading_zero_mask<5>(hashes);

        if (mask5 == 0)
            continue;

        alignas(32) std::array<uint32_t, 8> hashes_u32;
        _mm256_store_si256(reinterpret_cast<__m256i *>(&hashes_u32), hashes);

        // Check candidates for part 1.
        for (uint32_t m = mask5; m; m &= m - 1) {
            const int h = hashes_u32[std::countr_zero(m)];
            const auto h1 = (h >> 16) & 0xf;
            const auto h2 = (h >> 28) & 0xf;

            if (characters < 8) {
                password1 = password1 << 4 | h1;
                characters++;
            }

            if (h1 < 8 && (password2_mask & (1 << h1)) == 0) {
                password2_mask |= 1 << h1;
                password2 |= h2 << (4 * (7 - h1));
            }
        }

        if (characters == 8 && password2_mask == 0xff)
            return {password1, password2};
    }
}

void run(std::string_view buf)
{
    auto [part1, part2] = solve(buf);
    fmt::print("{:08x}\n{:08x}\n", part1, part2);
}

}
