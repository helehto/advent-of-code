#include "common.h"

namespace aoc_2021_16 {

struct Packet {
    uint8_t version;
    uint8_t type;
    int64_t value = 0;
    small_vector<uint16_t, 4> operands;
};

template <size_t N>
constexpr size_t read_bits(std::string_view &buf)
{
    DEBUG_ASSERT(buf.size() >= N);
    static_assert(N > 0);
    static_assert(N < 8 * sizeof(size_t) - 1);

    if constexpr (N >= 4) {
        uint32_t v;
        memcpy(&v, buf.data(), 4);
        size_t hi = _pext_u32(__builtin_bswap32(v), 0x01010101);
        buf.remove_prefix(4);
        if constexpr (N > 4)
            return hi << (N - 4) | read_bits<N - 4>(buf);
        else
            return hi;
    } else {
        size_t result = 0;
        for (size_t i = 0; i < N; ++i)
            result |= buf[i] << (N - i - 1);
        buf.remove_prefix(N);
        return result;
    }
}

constexpr size_t read_literal(std::string_view &buf)
{
    for (int64_t value = 0;;) {
        const bool done = read_bits<1>(buf) == 0;
        value = value << 4 | read_bits<4>(buf);
        if (done)
            return value;
    }
}

constexpr std::pair<int, size_t> parse_packet(std::vector<Packet> &storage,
                                              std::string_view buf);

constexpr void
parse_operand_packet(std::vector<Packet> &storage, Packet &packet, std::string_view &buf)
{
    if (read_bits<1>(buf) == 0) {
        const auto sublength = read_bits<15>(buf);
        for (std::string_view subdata = buf.substr(0, sublength); !subdata.empty();) {
            auto [operand, sublength] = parse_packet(storage, subdata);
            ASSERT(operand);
            subdata = subdata.substr(sublength);
            packet.operands.push_back(operand);
        }
        buf.remove_prefix(sublength);
    } else {
        const size_t num_subpackets = read_bits<11>(buf);
        for (size_t i = 0; i < num_subpackets; ++i) {
            auto [operand, sublength] = parse_packet(storage, buf);
            ASSERT(operand);
            packet.operands.push_back(operand);
            buf.remove_prefix(sublength);
        }
    }
}

constexpr std::pair<int, size_t> parse_packet(std::vector<Packet> &storage,
                                              std::string_view buf)
{
    if (buf.empty())
        return std::pair(-1, 0);

    const char *start = buf.data();

    const int result_index = storage.size();
    auto &packet = storage.emplace_back();
    packet.version = read_bits<3>(buf);
    packet.type = read_bits<3>(buf);

    if (packet.type == 4)
        packet.value = read_literal(buf);
    else
        parse_operand_packet(storage, packet, buf);

    return {result_index, buf.data() - start};
}

constexpr std::vector<Packet> parse_packets(const std::string_view bin)
{
    std::vector<Packet> storage;
    storage.reserve(bin.size());
    parse_packet(storage, bin);
    return storage;
}

constexpr std::array<char, 4> nibble_to_binary[] = {
    {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 1, 0, 0}, {0, 1, 0, 1},
    {0, 1, 1, 0}, {0, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}, {1, 0, 1, 1},
    {1, 1, 0, 0}, {1, 1, 0, 1}, {1, 1, 1, 0}, {1, 1, 1, 1},
};

constexpr std::string hex_to_binary(const std::string_view input)
{
    ASSERT(input.size() % 2 == 0);

    std::string result;
    result.resize(4 * input.size());
    for (char *p = result.data(); const char c : input) {
        const int nibble_value = (c >= '0' && c <= '9' ? c - '0' : c - 'A' + 10);
        memcpy(p, &nibble_to_binary[nibble_value], 4);
        p += 4;
    }

    return result;
}

constexpr int version_sum(std::span<const Packet> packets, const size_t index = 0)
{
    const Packet &p = packets[index];
    std::span<const uint16_t> op = p.operands;

    if (p.type == 4)
        return p.version;

    return std::ranges::fold_left(op, p.version, λab(a + version_sum(packets, b)));
}

constexpr int64_t eval(std::span<const Packet> packets, const size_t index = 0)
{
    const Packet &p = packets[index];
    std::span<const uint16_t> op = p.operands;

    switch (p.type) {
    case 0:
        return std::ranges::fold_left(op, 0, λab(a + eval(packets, b)));
    case 1:
        return std::ranges::fold_left(op, 1, λab(a * eval(packets, b)));
    case 2:
        return std::ranges::fold_left(op, INT64_MAX, λab(std::min(a, eval(packets, b))));
    case 3:
        return std::ranges::fold_left(op, INT64_MIN, λab(std::max(a, eval(packets, b))));
    case 4:
        return p.value;
    case 5:
        return eval(packets, op[0]) > eval(packets, op[1]);
    case 6:
        return eval(packets, op[0]) < eval(packets, op[1]);
    case 7:
        return eval(packets, op[0]) == eval(packets, op[1]);
    default:
        ASSERT_MSG(false, "p with unknown type {}!?", p.type);
    }
}

void run(std::string_view buf)
{
    const std::string bin = hex_to_binary(buf);
    const std::vector<Packet> packets = parse_packets(bin);
    fmt::print("{}\n", version_sum(packets));
    fmt::print("{}\n", eval(packets));
}

}
