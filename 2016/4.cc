#include "common.h"

namespace aoc_2016_4 {

struct ParsedLine {
    std::string_view name;
    uint16_t sector_id;
    std::array<char, 5> checksum;
};

constexpr ParsedLine parse_line(std::string_view s)
{
    // This assumes that each line ends in "-NNN[xxxxx]", which is the case for
    // my input.

    ASSERT(s.size() >= 11);
    const char *p = s.data() + s.size() - 11;
    ASSERT(*p == '-');
    p++;

    uint16_t sector_id = 0;
    ASSERT(p[0] >= '0' && p[0] <= '9');
    ASSERT(p[1] >= '0' && p[1] <= '9');
    ASSERT(p[2] >= '0' && p[2] <= '9');
    sector_id = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
    p += 3;

    ASSERT(*p == '[');
    p++;

    std::array<char, 5> checksum;
    for (int i = 0; i < 5; i++, p++) {
        ASSERT(*p >= 'a' && *p <= 'z');
        checksum[i] = *p;
    }
    ASSERT(*p == ']');

    return {s.substr(0, s.size() - 11), sector_id, checksum};
}

/// Potentially insert `value` into `a` such that it remains sorted in
/// ascending order.
constexpr void insert_sorted(std::span<uint16_t, 5> a, uint16_t value)
{
    DEBUG_ASSERT(std::ranges::is_sorted(a));

    if (value < a[2]) {
        if (value < a[0]) {
            a[4] = a[3];
            a[3] = a[2];
            a[2] = a[1];
            a[1] = a[0];
            a[0] = value;
        } else if (value < a[1]) {
            a[4] = a[3];
            a[3] = a[2];
            a[2] = a[1];
            a[1] = value;
        } else {
            a[4] = a[3];
            a[3] = a[2];
            a[2] = value;
        }
    } else {
        if (value < a[3]) {
            a[4] = a[3];
            a[3] = value;
        } else if (value < a[4]) {
            a[4] = value;
        }
    }
}

static void for_each_valid_room(std::string_view buf, auto &&f)
{
    // In each u16, the high byte is an inverted count (0, 1, 2, ... -> 0xffXX,
    // 0xfeXX, 0xfdXX, ...), and the low byte the ASCII value of the letter
    // itself. This gives a natural order by decreasing count with ties broken
    // by letter.
    //
    // The table has 256 entries so we can update histogram[c] for any char c
    // without a bounds check, but only 'a'..'z' really matter here.
    std::array<uint16_t, 256> histogram{};
    uint16_t *letters = &histogram['a'];

    // 32 rather than 26 to round it up to a full SIMD vector.
    std::array<uint16_t, 32> letters_init;
    for (size_t i = 0; i < 26; ++i)
        letters_init[i] = 0xff00 | ('a' + i);
    for (size_t i = 26; i < 32; ++i)
        letters_init[i] = 0xffff;

    while (true) {
        const size_t nl = buf.find('\n');
        if (nl == std::string_view::npos)
            break;

        std::string_view line = buf.substr(0, nl);
        buf.remove_prefix(nl + 1);

        std::ranges::copy(letters_init, letters);
        auto [name, sector_id, checksum] = parse_line(line);

        {
            size_t i = 0;
            for (; i + 3 < name.size(); i += 4) {
                histogram[name[i + 0]] -= 0x100;
                histogram[name[i + 1]] -= 0x100;
                histogram[name[i + 2]] -= 0x100;
                histogram[name[i + 3]] -= 0x100;
            }
            for (; i < name.size(); ++i)
                histogram[name[i]] -= 0x100;
        }

        std::array<uint16_t, 5> mins;
        mins.fill(0xffff);
        for (size_t i = 0; i < 26; ++i)
            insert_sorted(mins, letters[i]);

        std::array<char, 5> expected_checksum;
        for (size_t i = 0; i < 5; ++i)
            expected_checksum[i] = mins[i] & 0xff;

        if (checksum == expected_checksum)
            f(name, sector_id);
    }
}

static bool is_north_pole(std::string_view name, uint16_t sector_id)
{
    // Precomputed table of "north" shifted backwards by 0..25 letters, so we can
    // search the encrypted room names directly.
    static constexpr auto north_by_key = [] {
        std::array<std::array<char, 5>, 26> table;
        table[0] = {'n', 'o', 'r', 't', 'h'};
        for (size_t i = 1; i < 26; ++i)
            for (size_t j = 0; j < 5; ++j)
                table[i][j] = (table[i - 1][j] - 'a' + 25) % 26 + 'a';
        return table;
    }();

    std::string_view north(north_by_key[sector_id % 26].data(), 5);
    return name.find(north) != std::string::npos;
}

void run(std::string_view buf)
{
    int sector_id_sum = 0;
    int north_sector_id = -1;

    for_each_valid_room(buf, [&](std::string_view name, uint16_t sector_id) {
        sector_id_sum += sector_id;
        if (is_north_pole(name, sector_id))
            north_sector_id = sector_id;
    });

    fmt::print("{}\n{}\n", sector_id_sum, north_sector_id);
}

}
