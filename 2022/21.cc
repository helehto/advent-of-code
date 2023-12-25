#include "common.h"
#include <unordered_map>

namespace aoc_2022_21 {

enum {
    OP_CONSTANT = 1,
    OP_VARIABLE,
    OP_ADD,
    OP_MUL,
    OP_SUB,
    OP_DIV,
};

enum : int64_t {
    NO_VALUE = INT64_MIN,
};

struct Monkey {
    uint16_t type;
    union {
        uint16_t op[2];
        int16_t constant;
    };
};

struct Input {
    std::vector<Monkey> monkeys;
    size_t humn_index;
    size_t root_index;
};

static Input parse_monkeys(FILE *f)
{
    struct ParsedMonkey {
        char name[5];
        uint8_t type;
        union {
            char op[2][5];
            int constant;
        };
    };

    std::vector<ParsedMonkey> parsed_monkeys;
    std::string s;
    for (int i = 0; getline(f, s); i++) {
        auto &m = parsed_monkeys.emplace_back();
        char type;
        if (sscanf(s.c_str(), "%4s: %d", m.name, &m.constant) == 2) {
            m.type = OP_CONSTANT;
        } else if (sscanf(s.c_str(), "%4s: %4s %c %4s", m.name, m.op[0], &type,
                          m.op[1]) == 4) {
            if (type == '+')
                m.type = OP_ADD;
            else if (type == '-')
                m.type = OP_SUB;
            else if (type == '*')
                m.type = OP_MUL;
            else if (type == '/')
                m.type = OP_DIV;
        }
    }

    std::unordered_map<std::string_view, int> name_map;
    name_map.reserve(parsed_monkeys.size());
    for (size_t i = 0; i < parsed_monkeys.size(); i++)
        name_map[parsed_monkeys[i].name] = i;

    Input result;
    result.monkeys.reserve(parsed_monkeys.size());
    for (size_t i = 0; i < parsed_monkeys.size(); i++) {
        const auto &pm = parsed_monkeys[i];
        auto &m = result.monkeys.emplace_back();
        m.type = pm.type;
        if (pm.type == OP_CONSTANT) {
            m.constant = pm.constant;
        } else {
            m.op[0] = name_map.at(pm.op[0]);
            m.op[1] = name_map.at(pm.op[1]);
        }
    }

    result.root_index = name_map.at("root");
    result.humn_index = name_map.at("humn");
    return result;
}

static std::vector<int64_t> evaluate_tree(const Input &input)
{
    // Build reverse dependencies and initial value set.
    std::vector<int16_t> rdeps(input.monkeys.size(), -1);
    std::vector<int64_t> values(input.monkeys.size(), NO_VALUE);
    for (size_t i = 0; const auto &m : input.monkeys) {
        if (m.type != OP_CONSTANT && m.type != OP_VARIABLE) {
            rdeps[m.op[0]] = i;
            rdeps[m.op[1]] = i;
        } else if (m.type == OP_CONSTANT) {
            values[i] = m.constant;
        }
        i++;
    }

    // Build the initial queue for topological sort.
    std::vector<uint16_t> q;
    q.reserve(rdeps.size());
    for (size_t i = 0; i < rdeps.size(); i++)
        if (input.monkeys[i].type == OP_CONSTANT)
            q.push_back(rdeps[i]);

    // Topological sort.
    for (size_t i = 0; i < q.size(); i++) {
        const auto j = q[i];
        if (values[j] != NO_VALUE)
            continue;

        const auto &m = input.monkeys[j];
        const auto v1 = values[m.op[0]];
        const auto v2 = values[m.op[1]];
        if (v1 == NO_VALUE || v2 == NO_VALUE)
            continue;

        if (m.type == OP_ADD)
            values[j] = v1 + v2;
        else if (m.type == OP_SUB)
            values[j] = v1 - v2;
        else if (m.type == OP_MUL)
            values[j] = v1 * v2;
        else if (m.type == OP_DIV)
            values[j] = v1 / v2;

        if (rdeps[j] >= 0)
            q.push_back(rdeps[j]);
    }

    return values;
}

static int64_t part1(const Input &input)
{
    return evaluate_tree(input)[input.root_index];
}

static int64_t part2(const Input &input)
{
    const auto &m = input.monkeys;
    const auto &root = m[input.root_index];
    const auto v = evaluate_tree(input);
    const bool first_const_index = (v[root.op[0]] == NO_VALUE);
    int64_t k = v[root.op[first_const_index]];

    for (size_t curr = root.op[1 - first_const_index]; curr != input.humn_index;) {
        const auto &c = m[curr];
        const bool const_index = 1 - (v[c.op[1]] == NO_VALUE);
        const int64_t a = v[c.op[const_index]];

        if (m[curr].type == OP_ADD)
            k -= a;
        else if (m[curr].type == OP_MUL)
            k /= a;
        else if (m[curr].type == OP_SUB)
            k = const_index ? a + k : a - k;
        else if (m[curr].type == OP_DIV)
            k = const_index ? a * k : a / k;

        curr = c.op[1 - const_index];
    }

    return k;
}

void run(FILE *f)
{
    Input input = parse_monkeys(f);
    fmt::print("{}\n", part1(input));
    input.monkeys[input.humn_index].type = OP_VARIABLE;
    fmt::print("{}\n", part2(input));
}

}
