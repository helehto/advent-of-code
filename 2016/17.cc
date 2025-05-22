#include "common.h"
#include "md5.h"

namespace aoc_2016_17 {

enum {
    DOOR_U_OPEN = 1 << 0,
    DOOR_D_OPEN = 1 << 1,
    DOOR_L_OPEN = 1 << 2,
    DOOR_R_OPEN = 1 << 3,
};

constexpr int doors_from_hash(uint32_t u)
{
    int result = 0;

    if ((u & 0x00f0) >= 0x00b0)
        result |= DOOR_U_OPEN;
    if ((u & 0x000f) >= 0x000b)
        result |= DOOR_D_OPEN;
    if ((u & 0xf000) >= 0xb000)
        result |= DOOR_L_OPEN;
    if ((u & 0x0f00) >= 0x0b00)
        result |= DOOR_R_OPEN;

    return result;
}

static md5::Result md5_full(std::string_view s)
{
    const size_t len = s.size();
    const uint32_t lengths[8] = {
        static_cast<uint32_t>(len + 1),
        static_cast<uint32_t>(len + 1),
        static_cast<uint32_t>(len + 1),
        static_cast<uint32_t>(len + 1),
    };

    md5::Result r;
    r.a = _mm256_set1_epi32(0x67452301);
    r.b = _mm256_set1_epi32(0xefcdab89);
    r.c = _mm256_set1_epi32(0x98badcfe);
    r.d = _mm256_set1_epi32(0x10325476);

    for (; s.size() >= 64; s.remove_prefix(64)) {
        md5::Block8x8x64 m{};
        for (size_t i = 0; i < 4; ++i)
            memcpy(&m.data[i * 64], s.data(), 64);
        r = md5::do_block_avx2(m, r.a, r.b, r.c, r.d);
    }

    md5::Block8x8x64 m{};
    std::optional<size_t> x80_offset;

    for (size_t i = 0; i < 4; ++i) {
        memcpy(&m.data[i * 64], s.data(), s.size());
        m.data[i * 64 + s.size()] = "UDLR"[i];
    }

    if (s.size() >= 55) {
        // This messy logic is due to adding U/D/L/R to the string, which means
        // that we might need to push the 0x80 byte into a separate block.
        if (s.size() < 63) {
            for (size_t i = 0; i < 4; ++i)
                m.data[i * 64 + s.size() + 1] = 0x80;
        } else {
            x80_offset = 0;
        }
        r = md5::do_block_avx2(m, r.a, r.b, r.c, r.d);
        m = {};
    } else {
        x80_offset = s.size() + 1;
    }

    prepare_final_blocks(m, x80_offset, lengths);
    return md5::do_block_avx2(m, r.a, r.b, r.c, r.d);
}

static inplace_vector<std::tuple<Vec2i, std::string, uint32_t>, 4>
get_neighbors(Vec2i p, std::string str, uint32_t door_mask)
{
    auto h = md5_full(str).to_arrays()[0];
    inplace_vector<std::tuple<Vec2i, std::string, uint32_t>, 4> result;

    if ((door_mask & DOOR_U_OPEN) && p.y > 0)
        result.emplace_back(p + Vec2i{0, -1}, str + 'U', doors_from_hash(h[0]));
    if ((door_mask & DOOR_D_OPEN) && p.y < 3)
        result.emplace_back(p + Vec2i{0, +1}, str + 'D', doors_from_hash(h[1]));
    if ((door_mask & DOOR_L_OPEN) && p.x > 0)
        result.emplace_back(p + Vec2i{-1, 0}, str + 'L', doors_from_hash(h[2]));
    if ((door_mask & DOOR_R_OPEN) && p.x < 3)
        result.emplace_back(p + Vec2i{+1, 0}, str + 'R', doors_from_hash(h[3]));

    return result;
}

void run(std::string_view buf)
{
    std::vector<std::tuple<int, Vec2i, std::string, int>> queue;
    queue.emplace_back(0, Vec2i{}, std::string(buf),
                       doors_from_hash(md5_full(buf).to_arrays()[0][0]));

    std::optional<std::string> shortest;
    size_t longest = 0;

    for (size_t i = 0; i < queue.size(); ++i) {
        auto [d, p, str, mask] = queue[i];
        if (p == Vec2i{3, 3}) {
            if (!shortest)
                shortest = str;
            longest = std::max<size_t>(longest, str.size() - buf.size());
            continue;
        }
        for (auto [q, next_str, next_mask] : get_neighbors(p, str, mask))
            queue.emplace_back(d + 1, q, std::move(next_str), next_mask);
    }

    ASSERT_MSG(shortest.has_value(), "No path found!?");
    fmt::print("{}\n{}\n", shortest->substr(buf.size()), longest);
}

}
