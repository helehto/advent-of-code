#include <algorithm>
#include <aoc/string.h>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <hwy/highway.h>
#include <string_view>
#include <vector>

namespace hn = hwy::HWY_NAMESPACE;

static size_t split(std::string_view s, std::string_view *out, char c) noexcept
{
    using D = hn::ScalableTag<uint8_t>;
    constexpr D d;

    const hn::Vec<D> vsep = hn::Set(d, static_cast<uint8_t>(c));
    size_t count = 0;

    const char *p = s.data();
    const char *q = p + s.size();
    const char *curr_field_start = p;

    auto handle_chunk = [&](hn::Vec<D> vchars) {
        uint64_t mask = hn::BitsFromMask(d, hn::Eq(vchars, vsep));
        for (; mask != 0; mask &= mask - 1) {
            int offset = std::countr_zero(mask);
            *out++ = std::string_view(curr_field_start, p + offset - curr_field_start);
            count++;
            curr_field_start = p + offset + 1;
        }
    };

    for (; static_cast<size_t>(q - p) >= hn::Lanes(d); p += hn::Lanes(d))
        handle_chunk(hn::LoadU(d, reinterpret_cast<const uint8_t *>(p)));
    if (p != q) {
        std::array<uint8_t, hn::MaxLanes(d)> buffer{};
        memcpy(buffer.data(), p, q - p);
        handle_chunk(hn::LoadU(d, buffer.data()));
    }

    if (curr_field_start != q) {
        *out++ = std::string_view(curr_field_start, q - curr_field_start);
        count++;
    }

    return count;
}

std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, char c)
{
    out.clear();
    out.resize(std::count(s.begin(), s.end(), c) + 1);
    size_t n = split(s, out.data(), c);
    out.resize(n);
    return out;
}

std::vector<std::string_view> split_lines(std::string_view s)
{
    std::vector<std::string_view> lines;
    split(s, lines, '\n');
    return lines;
}
