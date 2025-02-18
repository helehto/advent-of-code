#include "common.h"
#include <algorithm>
#include <optional>
#include <variant>

namespace aoc_2022_13 {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
struct Packet {
    std::variant<int, std::vector<Packet>> data;
};
#pragma GCC diagnostic pop

static std::weak_ordering operator<=>(const Packet &a, const Packet &b)
{
    auto *ia = std::get_if<int>(&a.data);
    auto *ib = std::get_if<int>(&b.data);
    auto *va = std::get_if<std::vector<Packet>>(&a.data);
    auto *vb = std::get_if<std::vector<Packet>>(&b.data);

    if (ia && ib) {
        return *ia <=> *ib;
    } else if (va && vb) {
        return *va <=> *vb;
    } else if (ia && vb) {
        std::vector<Packet> ta{Packet{*ia}};
        return Packet{ta} <=> b;
    } else {
        std::vector<Packet> tb{Packet{*ib}};
        return a <=> Packet{tb};
    }
}

static std::optional<Packet> parse_packet(std::string_view &s)
{
    if (s.empty())
        return std::nullopt;

    if (isdigit(s.front())) {
        int v;
        auto r = std::from_chars(begin(s), end(s), v);
        ASSERT(r.ec == std::errc());
        s.remove_prefix(r.ptr - &s[0]);
        return Packet{v};
    }

    if (s.front() == '[') {
        std::vector<Packet> v;
        s.remove_prefix(1);

        while (true) {
            auto p = parse_packet(s);
            if (!p)
                break;
            ASSERT(!s.empty());
            v.emplace_back(*p);
            if (s.front() == ']')
                break;
            ASSERT(s.front() == ',');
            s.remove_prefix(1);
        }

        ASSERT(s.front() == ']');
        s.remove_prefix(1);
        return Packet{std::move(v)};
    }

    return std::nullopt;
}

static Packet parse_line(std::string_view s)
{
    auto p = parse_packet(s);
    ASSERT(p);
    ASSERT(s.empty());
    return *p;
}

void run(std::string_view buf)
{
    std::vector<Packet> packets;

    for (std::string_view s : split_lines(buf)) {
        if (!s.empty())
            packets.push_back(parse_line(s));
    }

    size_t sum = 0;
    for (size_t i = 0; i < packets.size(); i += 2) {
        if (packets[i] < packets[i + 1])
            sum += i / 2 + 1;
    }
    fmt::print("{}\n", sum);

    Packet a = parse_line("[[2]]");
    Packet b = parse_line("[[6]]");
    packets.push_back(a);
    packets.push_back(b);
    std::sort(begin(packets), end(packets));

    size_t key = 1;
    for (size_t i = 0; i < packets.size(); i++) {
        if ((packets[i] <=> a) == std::weak_ordering::equivalent ||
            (packets[i] <=> b) == std::weak_ordering::equivalent) {
            key *= i + 1;
        }
    }
    fmt::print("{}\n", key);
}

}
