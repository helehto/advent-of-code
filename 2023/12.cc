#include "common.h"
#include "dense_map.h"

namespace aoc_2023_12 {

struct u16_crc_hash {
    size_t operator()(uint16_t v) const { return _mm_crc32_u32(0, v); }
};

struct SearchParameters {
    std::string_view str;
    std::span<const uint32_t> block_sizes;
    dense_map<uint16_t, uint64_t, u16_crc_hash> &cache;
};

static uint64_t search(const SearchParameters &sp, size_t si, size_t bi)
{
    std::string_view s(sp.str.substr(si));

    size_t dots = 0;
    while (dots < s.size() && s[dots] == '.')
        dots++;
    s.remove_prefix(dots);
    si += dots;

    const uint16_t cache_key = si << 8 | bi;
    if (auto it = sp.cache.find(cache_key); it != sp.cache.end())
        return it->second;

    if (bi >= sp.block_sizes.size())
        return s.find('#') == std::string::npos ? 1 : 0;

    const auto block_size = sp.block_sizes[bi];
    if (block_size > s.size())
        return 0;

    uint64_t solutions = 0;
    const auto c = sp.str[si];
    ASSERT(c == '#' || c == '?');
    if (block_size <= s.size() &&
        s.substr(0, block_size).find('.') == std::string_view::npos &&
        (block_size == s.size() || s[block_size] != '#'))
        solutions += search(sp, si + std::min<size_t>(block_size + 1, s.size()), bi + 1);

    if (s.front() == '?')
        solutions += search(sp, si + 1, bi);

    sp.cache.emplace(cache_key, solutions);
    return solutions;
}

void run(std::string_view buf)
{
    uint64_t sum1 = 0;
    uint64_t sum2 = 0;

    small_vector<uint32_t> n;
    small_vector<uint32_t, 64> n2;
    dense_map<uint16_t, uint64_t, u16_crc_hash> cache;
    std::string s2;

    for (std::string_view sv : split_lines(buf)) {
        find_numbers(sv, n);
        std::string_view s = sv.substr(0, sv.find(' '));
        cache.clear();
        sum1 += search({s, n, cache}, 0, 0);

        s2.clear();
        fmt::format_to(std::back_inserter(s2), "{}?{}?{}?{}?{}", s, s, s, s, s);

        n2.clear();
        for (size_t i = 0; i < 5; ++i)
            n2.append_range(n);

        cache.clear();
        sum2 += search({s2, n2, cache}, 0, 0);
    }
    fmt::print("{}\n", sum1);
    fmt::print("{}\n", sum2);
}

}
