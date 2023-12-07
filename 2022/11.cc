#include "common.h"
#include <cstdint>
#include <fmt/core.h>
#include "common.h"
#include <numeric>

enum class Operation {
    add,
    mul,
    square,
};

struct Monkey11 {
    std::vector<int64_t> items;
    Operation op;
    int operand;
    int divisor;
    int targets[2];
    int64_t inspections = 0;
};

template <int Rounds, int ItemDivisor>
static int64_t run(std::vector<Monkey11> monkeys)
{
    int lcm = 1;
    for (const auto &m : monkeys)
        lcm = std::lcm(lcm, m.divisor);

    for (int i = 1; i <= Rounds; i++) {
        for (auto &m : monkeys) {
            for (auto w : m.items) {
                auto new_item = w;

                switch (m.op) {
                case Operation::add:
                    new_item += m.operand;
                    break;
                case Operation::mul:
                    new_item *= m.operand;
                    break;
                case Operation::square:
                    new_item = w * w;
                    break;
                }

                new_item = (new_item / ItemDivisor) % lcm;
                int target = m.targets[new_item % m.divisor == 0];
                monkeys[target].items.push_back(new_item);
            }

            m.inspections += m.items.size();
            m.items.clear();
        }
    }

    auto cmp = [](auto &a, auto &b) { return a.inspections > b.inspections; };
    std::sort(begin(monkeys), end(monkeys), cmp);
    return monkeys[0].inspections * monkeys[1].inspections;
}

static std::vector<Monkey11> parse_monkeys(FILE *f)
{
    std::vector<Monkey11> monkeys;

    std::string s;
    while (getline(f, s)) {
        Monkey11 &m = monkeys.emplace_back();

        // Starting items
        getline(f, s);
        m.items = find_numbers<int64_t>(s);

        // Operation
        {
            getline(f, s);

            auto op_str = s.substr(s.find(" = ") + 3);
            if (op_str == "old * old") {
                m.op = Operation::square;
            } else if (op_str.starts_with("old + ")) {
                m.op = Operation::add;
                std::from_chars(op_str.data() + 6, op_str.data() + op_str.size(),
                                m.operand);
            } else {
                m.op = Operation::mul;
                std::from_chars(op_str.data() + 6, op_str.data() + op_str.size(),
                                m.operand);
            }
        }

        // Test
        getline(f, s);
        m.divisor = find_numbers<int>(s)[0];

        // If true, if false:
        getline(f, s);
        m.targets[1] = find_numbers<int>(s)[0];
        getline(f, s);
        m.targets[0] = find_numbers<int>(s)[0];

        // Blank line
        getline(f, s);
    }

    return monkeys;
}

void run_2022_11(FILE *f)
{
    auto monkeys = parse_monkeys(f);
    fmt::print("{}\n", run<20, 3>(monkeys));
    fmt::print("{}\n", run<10000, 1>(std::move(monkeys)));
}
