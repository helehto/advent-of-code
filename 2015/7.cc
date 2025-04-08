#include "common.h"
#include "dense_map.h"

namespace aoc_2015_7 {

using namespace std::string_literals;

enum class GateType : uint8_t {
    passthru,
    and_,
    lshift,
    rshift,
    or_,
    not_,
};

struct Gate {
    int32_t op1;
    int32_t op2;
    GateType type;
};

template <typename T>
std::optional<T> try_from_chars(std::string_view s)
{
    T v;
    auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r != std::errc() || r.ptr == s.data())
        return std::nullopt;
    return v;
}

struct Input {
    std::vector<Gate> gates;
    dense_map<std::string_view, int> gate_index;
};

static Input parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);

    Input result;
    result.gates.reserve(lines.size());
    result.gate_index.reserve(lines.size());

    for (int i = 0; std::string_view line : lines) {
        auto rhs = strip(line.substr(line.find(" -> ") + 4));
        result.gate_index.emplace(rhs, i++);
    }

    for (int i = 0; std::string_view line : lines) {
        auto arrow = line.find(" -> ");
        ASSERT(arrow != std::string_view::npos);
        auto lhs = line.substr(0, arrow);
        int v;

        auto parse_op = [&](std::string_view s) {
            int32_t n;
            auto r = std::from_chars(s.data(), s.data() + s.size(), n);
            return (r.ec == std::errc() && r.ptr != s.data()) ? n
                                                              : ~result.gate_index.at(s);
        };

        if (lhs.starts_with("NOT")) {
            result.gates.push_back(Gate{
                .op1 = parse_op(lhs.substr(4)),
                .type = GateType::not_,
            });
        } else if (auto i = lhs.find(" AND "); i != std::string_view::npos) {
            result.gates.push_back(Gate{
                .op1 = parse_op(lhs.substr(0, i)),
                .op2 = parse_op(lhs.substr(i + 5)),
                .type = GateType::and_,
            });
        } else if (auto i = lhs.find(" LSHIFT "); i != std::string_view::npos) {
            result.gates.push_back(Gate{
                .op1 = parse_op(lhs.substr(0, i)),
                .op2 = parse_op(lhs.substr(i + 8)),
                .type = GateType::lshift,
            });
        } else if (auto i = lhs.find(" RSHIFT "); i != std::string_view::npos) {
            result.gates.push_back(Gate{
                .op1 = parse_op(lhs.substr(0, i)),
                .op2 = parse_op(lhs.substr(i + 8)),
                .type = GateType::rshift,
            });
        } else if (auto i = lhs.find(" OR "); i != std::string_view::npos) {
            result.gates.push_back(Gate{
                .op1 = parse_op(lhs.substr(0, i)),
                .op2 = parse_op(lhs.substr(i + 4)),
                .type = GateType::or_,
            });
        } else if (auto r = std::from_chars(lhs.data(), lhs.data() + lhs.size(), v);
                   r.ec == std::errc()) {
            result.gates.push_back(Gate{
                .op1 = v,
                .type = GateType::passthru,
            });
        } else {
            ASSERT(lhs.find(' ') == std::string_view::npos);
            result.gates.push_back(Gate{
                .op1 = ~result.gate_index.at(lhs),
                .type = GateType::passthru,
            });
        }

        ++i;
    }

    return result;
}

static int solve(const Input &input)
{
    const auto &gates = input.gates;
    const auto &gate_index = input.gate_index;

    std::vector<int16_t> num_unresolved_inputs(gates.size());
    for (size_t i = 0; i < gates.size(); ++i)
        num_unresolved_inputs[i] = (gates[i].op1 < 0) + (gates[i].op2 < 0);

    std::vector<std::vector<uint16_t>> outputs(gates.size());
    for (size_t i = 0; i < gates.size(); ++i) {
        if (gates[i].op1 < 0)
            outputs[~gates[i].op1].push_back(i);
        if (gates[i].op2 < 0)
            outputs[~gates[i].op2].push_back(i);
    }

    std::vector<int32_t> values(gates.size(), -1);
    std::vector<uint16_t> queue;
    for (size_t i = 0; i < gates.size(); ++i) {
        if (num_unresolved_inputs[i] == 0) {
            ASSERT(gates[i].type == GateType::passthru);
            queue.push_back(i);
        }
    }

    while (!queue.empty()) {
        size_t i = queue.back();
        queue.pop_back();

        if (values[i] >= 0)
            continue;

        const Gate &g = gates[i];

        const auto val1 = g.op1 >= 0 ? g.op1 : values[~g.op1];
        const auto val2 = g.op2 >= 0 ? g.op2 : values[~g.op2];

        switch (g.type) {
        case GateType::and_:
            values[i] = static_cast<uint16_t>(val1 & val2);
            break;
        case GateType::lshift:
            values[i] = static_cast<uint16_t>(val1 << val2);
            break;
        case GateType::rshift:
            values[i] = static_cast<uint16_t>(val1 >> val2);
            break;
        case GateType::or_:
            values[i] = static_cast<uint16_t>(val1 | val2);
            break;
        case GateType::not_:
            values[i] = static_cast<uint16_t>(~val1);
            break;
        case GateType::passthru:
            values[i] = static_cast<uint16_t>(val1);
            break;
        }

        for (uint16_t j : outputs[i])
            if (--num_unresolved_inputs[j] <= 0)
                queue.push_back(j);
        outputs[i].clear();
    }

    return values[gate_index.at("a")];
}

void run(std::string_view buf)
{
    Input input = parse_input(buf);

    auto a = solve(input);
    fmt::print("{}\n", a);

    input.gates[input.gate_index.at("b")] = Gate{
        .op1 = a,
        .type = GateType::passthru,
    };
    fmt::print("{}\n", solve(input));
}

}
