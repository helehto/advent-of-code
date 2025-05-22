#include "common.h"
#include "small_vector.h"
#include <optional>

namespace aoc_2022_13 {

struct Packet {
    // Each element in this element is either an index into the storage (if
    // positive), or the bitwise NOT of a constant (if negative).
    small_vector<int16_t, 8> data;
};

static std::weak_ordering compare(std::span<const Packet> storage,
                                  std::span<const int16_t> a,
                                  std::span<const int16_t> b);

static std::weak_ordering
compare(std::span<const Packet> storage, const int16_t a, const int16_t b)
{
    if (a >= 0 && b >= 0) {
        return compare(storage, storage[a].data, storage[b].data);
    } else if (a >= 0 && b < 0) {
        return compare(storage, storage[a].data, std::span(&b, 1));
    } else if (a < 0 && b >= 0) {
        return compare(storage, std::span(&a, 1), storage[b].data);
    } else {
        return ~a <=> ~b;
    }
}

static std::weak_ordering compare(std::span<const Packet> storage,
                                  std::span<const int16_t> a,
                                  std::span<const int16_t> b)
{
    return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(),
                                                  λxy(compare(storage, x, y)));
}

static std::optional<int> parse_packet(std::vector<Packet> &storage, std::string_view &s)
{
    if (s.empty())
        return std::nullopt;

    if (isdigit(s.front())) {
        int v;
        auto r = std::from_chars(begin(s), end(s), v);
        ASSERT(r.ec == std::errc());
        s.remove_prefix(r.ptr - &s[0]);
        return ~v;
    }

    if (s.front() == '[') {
        const int result = storage.size();
        storage.emplace_back();
        s.remove_prefix(1);

        while (true) {
            std::optional<int> p = parse_packet(storage, s);
            if (!p)
                break;
            ASSERT(!s.empty());
            storage[result].data.emplace_back(*p);
            if (s.front() == ']')
                break;
            ASSERT(s.front() == ',');
            s.remove_prefix(1);
        }

        ASSERT(s.front() == ']');
        s.remove_prefix(1);
        return result;
    }

    return std::nullopt;
}

static int parse_line(std::vector<Packet> &storage, std::string_view s)
{
    std::optional<int> p = parse_packet(storage, s);
    ASSERT(p);
    ASSERT(s.empty());
    return *p;
}

void run(std::string_view buf)
{
    const auto lines = split_lines(buf);

    std::vector<Packet> storage;
    storage.reserve(2 * std::ranges::count(buf, '['));

    std::vector<int> packets;
    packets.reserve(lines.size() + 2);

    for (std::string_view s : lines) {
        if (!s.empty())
            packets.push_back(parse_line(storage, s));
    }

    // Part 1:
    {
        size_t sum = 0;
        for (size_t i = 0; i < packets.size(); i += 2) {
            if (std::is_lt(compare(storage, packets[i], packets[i + 1])))
                sum += i / 2 + 1;
        }
        fmt::print("{}\n", sum);
    }

    // Part 2:
    {
        int a = parse_line(storage, "[[2]]");
        int b = parse_line(storage, "[[6]]");
        packets.push_back(a);
        packets.push_back(b);
        std::ranges::sort(packets, λxy(std::is_lt(compare(storage, x, y))));

        size_t key = 1;
        for (size_t i = 0; i < packets.size(); i++) {
            if (packets[i] == a || packets[i] == b)
                key *= i + 1;
        }
        fmt::print("{}\n", key);
    }
}

}
