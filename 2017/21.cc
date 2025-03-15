#include "common.h"

namespace aoc_2017_21 {

constexpr static int parse_2x2_rule(std::string_view s)
{
    return (s[0] == '#') << 0 | (s[1] == '#') << 1 | (s[3] == '#') << 2 |
           (s[4] == '#') << 3;
}

constexpr static int parse_3x3_rule(std::string_view s)
{
    return (s[0] == '#') << 0 | (s[1] == '#') << 1 | (s[2] == '#') << 2 |
           (s[4] == '#') << 3 | (s[5] == '#') << 4 | (s[6] == '#') << 5 |
           (s[8] == '#') << 6 | (s[9] == '#') << 7 | (s[10] == '#') << 8;
}

constexpr static int parse_4x4_rule(std::string_view s)
{
    return (s[0] == '#') << 0 | (s[1] == '#') << 1 | (s[2] == '#') << 2 |
           (s[3] == '#') << 3 | (s[5] == '#') << 4 | (s[6] == '#') << 5 |
           (s[7] == '#') << 6 | (s[8] == '#') << 7 | (s[10] == '#') << 8 |
           (s[11] == '#') << 9 | (s[12] == '#') << 10 | (s[13] == '#') << 11 |
           (s[15] == '#') << 12 | (s[16] == '#') << 13 | (s[17] == '#') << 14 |
           (s[18] == '#') << 15;
}

static std::pair<std::array<std::array<uint32_t, 3>, 16>, std::array<uint16_t, 512>>
parse_rules(std::string_view buf)
{
    std::array<std::array<uint32_t, 3>, 16> rules_2x2{};
    std::array<uint16_t, 512> rules_3x3{};

    std::vector<std::string_view> tmp;
    for (std::string_view line : split_lines(buf)) {
        auto arrow = line.find("=>");
        auto p = line.substr(0, arrow - 1);
        auto q = line.substr(arrow + 3);

        if (p.size() == 5) {
            auto rotate = [&](int x) {
                return (x & 1) << 1 | (x & 2) << 2 | (x & 4) >> 2 | (x & 8) >> 1;
            };

            auto flip = [&](int x) {
                return (x & 1) << 1 | (x & 2) >> 1 | (x & 4) << 1 | (x & 8) >> 1;
            };

            const int orig = parse_2x2_rule(p.data());

            std::array<uint32_t, 3> k;
            k[0] = (q[0] == '#' ? uint32_t(0x80) : 0) |
                   (q[1] == '#' ? uint32_t(0x80) << 8 : 0) |
                   (q[2] == '#' ? uint32_t(0x80) << 16 : 0);
            k[1] = (q[4] == '#' ? uint32_t(0x80) : 0) |
                   (q[5] == '#' ? uint32_t(0x80) << 8 : 0) |
                   (q[6] == '#' ? uint32_t(0x80) << 16 : 0);
            k[2] = (q[8] == '#' ? uint32_t(0x80) : 0) |
                   (q[9] == '#' ? uint32_t(0x80) << 8 : 0) |
                   (q[10] == '#' ? uint32_t(0x80) << 16 : 0);

            std::array<int, 4> rotations{orig};
            for (size_t i = 1; i < rotations.size(); ++i)
                rotations[i] = rotate(rotations[i - 1]);

            for (const int rule : rotations) {
                rules_2x2[rule] = k;
                rules_2x2[flip(rule)] = k;
            }
        } else {
            auto rotate = [&](int x) {
                return (x & 1) << 2 | (x & 2) << 4 | (x & 4) << 6 | (x & 8) >> 2 |
                       (x & 16) | (x & 32) << 2 | (x & 64) >> 6 | (x & 128) >> 4 |
                       (x & 256) >> 2;
            };

            auto flip = [&](int x) {
                return (x & 1) << 2 | (x & 2) | (x & 4) >> 2 | (x & 8) << 2 | (x & 16) |
                       (x & 32) >> 2 | (x & 64) << 2 | (x & 128) | (x & 256) >> 2;
            };

            const int orig = parse_3x3_rule(p.data());
            const int k = parse_4x4_rule(q.data());

            std::array<int, 4> rotations{orig};
            for (size_t i = 1; i < rotations.size(); ++i)
                rotations[i] = rotate(rotations[i - 1]);

            for (const int rule : rotations) {
                rules_3x3[rule] = k;
                rules_3x3[flip(rule)] = k;
            }
        }
    }

    return {rules_2x2, rules_3x3};
}

void run(std::string_view buf)
{
    auto [rules_2x2, rules_3x3] = parse_rules(buf);

    // Pre-compute the size of the largest grid required.
    constexpr size_t max_size = [&] {
        int n = 3;
        for (int i = 0; i < 18; ++i) {
            if (n % 2 == 0)
                n = 3 * n / 2;
            else if (n % 3 == 0)
                n = 4 * n / 3;
            else
                __builtin_unreachable();
        }
        return (n + 3) & ~3;
    }();

    Matrix<uint8_t> grid(max_size, max_size, 0);
    size_t size = 3;
    grid(0, 1) = 0x80;
    grid(1, 2) = 0x80;
    grid(2, 0) = 0x80;
    grid(2, 1) = 0x80;
    grid(2, 2) = 0x80;

    auto at16 = [&grid](size_t i, size_t j) -> uint16_t {
        return *reinterpret_cast<uint16_t *>(&grid(i, j));
    };

    auto expand_2x2_to_3x3 = [&] {
        const int new_size = 3 * size / 2;

        // Expand in-place by starting from the lowest block.
        for (ssize_t i = size - 2, ii = new_size - 3; i >= 0; i -= 2, ii -= 3) {
            // Each output block is 3x3, but we want to coalesce the three
            // byte-sized writes into a single 4-byte write; to do that we have
            // to keep track of the first byte (the most significant byte of
            // the 32-bit word) of each block, and preserve its value in the
            // subsequent iteration.
            std::array<uint32_t, 3> prev{};

            // Expand in-place by starting from the right-most block.
            for (ssize_t j = size - 2, jj = new_size - 3; j >= 0; j -= 2, jj -= 3) {
                // Figure out which rule to apply by merging the 2x2 bytes with
                // values 0x00 or 0x80 into a 4-bit value (0-15) using pext.
                const uint32_t block = at16(i, j) | at16(i + 1, j) << 16;
                const auto rule = rules_2x2[_pext_u32(block, 0x80808080)];

                // Write the three output rows according to the output rule,
                // while preserving the first byte of the previous output
                // block.
                const uint32_t r0 = prev[0] << 24 | rule[0];
                const uint32_t r1 = prev[1] << 24 | rule[1];
                const uint32_t r2 = prev[2] << 24 | rule[2];
                memcpy(&grid(ii + 0, jj), &r0, sizeof(r0));
                memcpy(&grid(ii + 1, jj), &r1, sizeof(r1));
                memcpy(&grid(ii + 2, jj), &r2, sizeof(r2));
                prev = rule;
            }
        }

        return new_size;
    };

    auto expand_3x3_to_4x4 = [&] {
        const int new_size = 4 * size / 3;

        // Expand in-place by starting from the lowest block.
        for (ssize_t i = size - 3, ii = new_size - 4; i >= 0; i -= 3, ii -= 4) {
            // Expand in-place by starting from the right-most block.
            for (ssize_t j = size - 3, jj = new_size - 4; j >= 0; j -= 3, jj -= 4) {
                // Figure out which rule to apply by merging the 3x3 bytes with
                // values 0x00 or 0x80 into a 9-bit value using pext. This is a
                // bit tricky since 3x3 bytes won't fit into a 64-bit word, so
                // build the value in two steps using two pexts - one for the
                // first row and one for the second and third rows - and
                // combining the output.
                uint32_t row0, row1, row2;
                memcpy(&row0, &grid(i + 0, j), sizeof(row0));
                memcpy(&row1, &grid(i + 1, j), sizeof(row1));
                memcpy(&row2, &grid(i + 2, j), sizeof(row2));
                uint64_t mask0 = _pext_u64(row0, 0x808080);
                // Since all bytes in row1 is either 0x00 or 0x80, or'ing the
                // pext mask with 7 always inserts three zeroes, shifting the
                // output value to the left by 3 for free.
                uint64_t mask12 = _pext_u64(row1 | static_cast<uint64_t>(row2) << 32,
                                            0x00808080'00808080 | 7);
                int rule = rules_3x3[mask0 | mask12];

                // Expand the output rule to four rows of four 0x00 or 0x80
                // bytes using pdep.
                const uint32_t r0 = _pdep_u32(rule, 0x80808080);
                const uint32_t r1 = _pdep_u32(rule >> 4, 0x80808080);
                const uint32_t r2 = _pdep_u32(rule >> 8, 0x80808080);
                const uint32_t r3 = _pdep_u32(rule >> 12, 0x80808080);
                memcpy(&grid(ii + 0, jj), &r0, sizeof(r0));
                memcpy(&grid(ii + 1, jj), &r1, sizeof(r1));
                memcpy(&grid(ii + 2, jj), &r2, sizeof(r2));
                memcpy(&grid(ii + 3, jj), &r3, sizeof(r3));
            }
        }

        return new_size;
    };

    for (int iters = 1; iters <= 18; ++iters) {
        if (size % 2 == 0)
            size = expand_2x2_to_3x3();
        else
            size = expand_3x3_to_4x4();

        if (iters == 5 || iters == 18) {
            int on = 0;
            for (size_t i = 0; i < size; ++i)
                for (size_t j = 0; j < size; ++j)
                    on += !!grid(i, j);
            fmt::print("{}\n", on);
        }
    }
}

}
