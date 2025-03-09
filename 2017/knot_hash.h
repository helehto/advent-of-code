#pragma once

#include <array>
#include <cstdint>
#include <numeric>
#include <span>
#include <utility>

struct KnotHash {
    size_t pos = 0;
    size_t skip = 0;
    std::array<uint8_t, 256> ring;

    KnotHash() { std::iota(ring.begin(), ring.end(), 0); }

    void sparse_round(std::span<const uint8_t> l)
    {
        for (size_t n : l) {
            for (size_t i = 0; i < n / 2; ++i)
                std::swap(ring[(pos + i) & 0xff], ring[(pos + n - i - 1) & 0xff]);

            pos += n + skip;
            skip++;
        }
    }

    inline std::array<uint8_t, 16> as_bytes(std::span<const uint8_t> lengths)
    {
        static constexpr uint8_t extra[]{17, 31, 73, 47, 23};

        for (int i = 0; i < 64; ++i) {
            sparse_round(lengths);
            sparse_round(extra);
        }

        std::array<uint8_t, 16> result;
        for (size_t i = 0; i < 16; ++i) {
            auto p = &ring[16 * i];
            result[i] = std::reduce(p, p + 16, 0, std::bit_xor<uint8_t>{});
        }

        return result;
    }
};

inline std::array<char, 33> as_hex(const std::array<uint8_t, 16> &hash)
{
    std::array<char, 33> result;
    result.back() = '\0';
    for (size_t i = 0; i < hash.size(); ++i) {
        result[2 * i] = "0123456789abcdef"[hash[i] >> 4];
        result[2 * i + 1] = "0123456789abcdef"[hash[i] & 0xf];
    }

    return result;
}
