#include "common.h"

namespace aoc_2024_19 {

struct PrefixTree {
    std::unique_ptr<std::array<uint16_t, 8>[]> storage;
    uint32_t size = 0;
    static constexpr auto occupied_mask = UINT16_C(1) << 15;

    PrefixTree()
        : storage(std::make_unique_for_overwrite<std::array<uint16_t, 8>[]>(1 << 15))
    {
        storage[size++].fill(0);
    }

    /// Takes one step in the prefix tree given a node index and a 3-bit input
    /// `bits`. The returned pair signifies the next node index, and whether
    /// the current bit string plus `bits` is in the tree.
    std::pair<size_t, bool> step(size_t index, int bits) const
    {
        const auto entry = storage[index][bits];
        const bool hit = (entry & occupied_mask) != 0;
        return std::pair(entry & ~occupied_mask, hit);
    }

    /// Insert the entry `seq` into the prefix tree. The input is a bit string
    /// that represents a sequence of 3-bit numbers, each of which corresponds
    /// to a color (see color_to_3bit), terminated by 0.
    void insert(uint64_t seq)
    {
        size_t index = 0;
        while (true) {
            auto seq_next = seq >> 3;
            auto &node = storage[index];

            if (seq_next == 0) {
                node[seq] |= occupied_mask;
                return;
            }

            size_t entry = node[seq & 7];
            if ((entry & ~occupied_mask) == 0) {
                entry |= size;
                node[seq & 7] = entry;
                storage[size++].fill(0);
            }
            index = entry & ~occupied_mask;
            seq = seq_next;
        }
    }
};

/// Table mapping the five valid colors (b, g, r, u, w) to a 3-bit number.
constexpr auto color_to_3bit = []() consteval {
    // Note: all valid colors are mapped to non-zero, since 0 signifies an
    // unused entry in the prefix tree.
    std::array<uint8_t, 256> result{};
    result['r'] = 0b001;
    result['b'] = 0b010;
    result['g'] = 0b011;
    result['w'] = 0b100;
    result['u'] = 0b101;
    return result;
}();

static int64_t solve(std::string_view s, const PrefixTree &designs)
{
    std::vector<int64_t> cache(s.size(), 0);

    for (size_t i = s.size(); i--;) {
        bool hit;
        size_t index = 0;
        size_t j = i;
        int64_t result = 0;
        do {
            std::tie(index, hit) = designs.step(index, color_to_3bit[s[j++]]);
            if (hit) {
                if (j >= s.size()) {
                    result++;
                    break;
                }
                result += cache[j];
            }
        } while (index != 0 && j < s.size());
        cache[i] = result;
    }

    return cache[0];
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::string_view> tmp;
    split(lines[0], tmp, ',');
    for (auto &d : tmp)
        d = strip(d);

    PrefixTree designs;

    for (std::string_view t : tmp) {
        uint64_t d = 0;
        for (size_t i = 0, shift = 0; i < t.size(); ++i, shift += 3)
            d |= static_cast<uint64_t>(color_to_3bit[t[i]]) << shift;
        designs.insert(d);
    }

    int64_t s1 = 0;
    int64_t s2 = 0;
    for (size_t i = 2; i < lines.size(); i++) {
        auto r = solve(lines[i], designs);
        s1 += r > 0;
        s2 += r;
    }
    fmt::print("{}\n{}\n", s1, s2);
}

}
