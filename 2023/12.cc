#include "common.h"
#include <unordered_map>

static std::unordered_map<uint16_t, uint64_t> cache;

struct SearchParameters12 {
    std::string_view str;
    std::span<const uint32_t> block_sizes;
};

static uint64_t search(const SearchParameters12 &sp, size_t si, size_t bi)
{
    std::string_view s(sp.str.substr(si));

    size_t dots = 0;
    while (dots < s.size() && s[dots] == '.')
        dots++;
    s.remove_prefix(dots);
    si += dots;

    const uint16_t cache_key = si << 8 | bi;
    if (auto it = cache.find(cache_key); it != cache.end())
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
        s[block_size] != '#')
        solutions += search(sp, si + std::min<size_t>(block_size + 1, s.size()), bi + 1);

    if (s.front() == '?')
        solutions += search(sp, si + 1, bi);

    cache.emplace(cache_key, solutions);
    return solutions;
}

void run_2023_12(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    uint64_t sum1 = 0;
    uint64_t sum2 = 0;

    for (std::string_view sv : lines) {
        auto n = find_numbers<uint32_t>(sv);
        std::string q(sv.substr(0, sv.find(' ')));
        cache.clear();
        auto solutions = search({q, n}, 0, 0);
        sum1 += solutions;

        auto s2 = std::string(q) + "?" + q + "?" + q + "?" + q + "?" + q;
        std::vector<uint32_t> n2;
        n2.insert(n2.end(), n.begin(), n.end());
        n2.insert(n2.end(), n.begin(), n.end());
        n2.insert(n2.end(), n.begin(), n.end());
        n2.insert(n2.end(), n.begin(), n.end());
        n2.insert(n2.end(), n.begin(), n.end());

        cache.clear();
        auto sols2 = search({s2, n2}, 0, 0);
        sum2 += sols2;
    }
    fmt::print("{}\n", sum1);
    fmt::print("{}\n", sum2);
}
