#include "common.h"
#include "knot_hash.h"

namespace aoc_2017_14 {

void run(std::string_view buf)
{
    Matrix<bool> grid(128, 128);

    int used_squares = 0;
    for (int i = 0; i < 128; ++i) {
        char input[16];
        const auto len = fmt::format_to(input, "{}-{}", buf, i) - input;

        std::span input_u8(reinterpret_cast<uint8_t *>(input), len);
        const auto hash = KnotHash{}.as_bytes(input_u8);

        auto *hash_u64 = reinterpret_cast<const uint64_t *>(hash.data());
        used_squares += std::popcount(hash_u64[0]) + std::popcount(hash_u64[1]);

        for (int j = 0; j < 16; ++j) {
            auto dep = _pdep_u64(hash[j], uint64_t(0x0101010101010101));
            auto dep_rev = std::byteswap(dep);
            *reinterpret_cast<uint64_t *>(&grid(i, 8 * j)) = dep_rev;
        }
    }
    fmt::print("{}\n", used_squares);

    Matrix<bool> visited(grid.rows, grid.cols, false);
    int regions = 0;
    std::vector<Vec2i> queue;
    auto flood = [&](Vec2i p) {
        for (queue = {p}; !queue.empty();) {
            auto u = queue.back();
            queue.pop_back();
            visited(u) = true;
            for (auto v : neighbors4(grid, u))
                if (grid(v) && !visited(v))
                    queue.push_back(v);
        }
    };

    for (auto u : grid.ndindex<int>()) {
        if (grid(u) && !visited(u)) {
            flood(u);
            ++regions;
        }
    }
    fmt::print("{}\n", regions);
}

}
