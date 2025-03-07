#include "common.h"

namespace aoc_2017_10 {

template <size_t Rounds>
static std::array<uint8_t, 256> knot_hash(std::span<uint8_t> lengths)
{
    size_t pos = 0;
    size_t skip = 0;
    std::array<uint8_t, 256> ring;
    std::iota(ring.begin(), ring.end(), 0);

    for (size_t i = 0; i < Rounds; ++i) {
        for (size_t n : lengths) {
            for (size_t i = 0; i < n / 2; ++i)
                std::swap(ring[(pos + i) & 0xff], ring[(pos + n - i - 1) & 0xff]);

            pos += n + skip;
            skip++;
        }
    }

    return ring;
}

static int part1(std::string_view buf)
{
    auto lengths = find_numbers<uint8_t>(buf);
    auto ring = knot_hash<1>(lengths);
    return ring[0] * ring[1];
}

static std::array<char, 33> part2(std::string_view buf)
{
    constexpr std::array<uint8_t, 5> extra{17, 31, 73, 47, 23};

    std::vector<uint8_t> lengths;
    lengths.reserve(buf.size() + extra.size());
    std::ranges::copy(buf, std::back_inserter(lengths));
    std::ranges::copy(extra, std::back_inserter(lengths));

    auto sparse = knot_hash<64>(lengths);

    std::array<uint8_t, 16> dense;
    for (size_t i = 0; i < 16; ++i)
        dense[i] = std::reduce(sparse.begin() + 16 * i, sparse.begin() + 16 * (i + 1), 0,
                               std::bit_xor<uint8_t>{});

    std::array<char, 33> hex;
    hex.back() = '\0';
    for (size_t i = 0; i < dense.size(); ++i) {
        hex[2 * i] = "0123456789abcdef"[dense[i] >> 4];
        hex[2 * i + 1] = "0123456789abcdef"[dense[i] & 0xf];
    }

    return hex;
}

void run(std::string_view buf)
{
    fmt::print("{}\n", part1(buf));
    fmt::print("{}\n", part2(buf).data());
}

}
