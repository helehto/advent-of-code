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

    void sparse_round(size_t n)
    {
        if (pos + n <= ring.size()) {
            // Entire range is in bounds.
            std::reverse(ring.begin() + pos, ring.begin() + pos + n);
        } else if (pos + n / 2 > 256) {
            // Second half of the range (partially) wraps around.
            size_t i = 0;
            for (; i < 256 - pos; ++i)
                std::swap(ring[pos + i], ring[pos + n - i - 1 - 256]);
            for (; i < n / 2; ++i)
                std::swap(ring[pos + i - 256], ring[pos + n - i - 1 - 256]);
        } else {
            // First half of the range (partially) wraps around, second half is
            // in bounds. (FIXME: This can be split into two cases just like
            // above to avoid masking for each iteration, but the indexing is
            // fiddly so I won't bother right now.)
            for (size_t i = 0; i < n / 2; ++i)
                std::swap(ring[(pos + i) & 0xff], ring[(pos + n - i - 1) & 0xff]);
        }

        pos = (pos + n + skip) & 0xff;
        skip++;
    }

    inline std::array<uint8_t, 16> as_bytes(std::span<const uint8_t> lengths)
    {
        for (int i = 0; i < 64; ++i) {
            for (const size_t n : lengths)
                sparse_round(n);
            sparse_round(17);
            sparse_round(31);
            sparse_round(73);
            sparse_round(47);
            sparse_round(23);
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
