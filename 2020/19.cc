#include "common.h"
#include "inplace_vector.h"

namespace aoc_2020_19 {

enum { RULE_LITERAL, RULE_ALT };

struct Rule {
    int type;
    inplace_vector<uint8_t, 4> alternatives[2];
};

static std::pair<int, Rule> parse_rule(std::string_view line)
{
    int rule_num = -1;
    auto r = std::from_chars(line.begin(), line.end(), rule_num);
    ASSERT(r.ec == std::errc());
    ASSERT(rule_num >= 0);
    line.remove_prefix(line.find(' ') + 1);

    std::vector<std::string_view> alternatives;
    std::vector<std::string_view> tokens;

    Rule result;

    if (line[0] == '"') {
        result.type = RULE_LITERAL;
        result.alternatives[0].push_back(line[1]);
    } else {
        result.type = RULE_ALT;
        split(line, alternatives, '|');
        ASSERT(alternatives.size() <= 2);
        for (size_t i = 0; std::string_view alternative : alternatives) {
            for (std::string_view s : split(strip(alternative), tokens, ' ')) {
                s = strip(s);
                int n = 0;
                std::from_chars(s.data(), s.data() + s.size(), n);
                result.alternatives[i].push_back(n);
            }
            ++i;
        }
    }

    return {rule_num, result};
}

static bool
match(std::string_view s, std::span<const Rule> rules, small_vector_base<int> &rule_seq)
{
    if (s.empty() || rule_seq.empty())
        return s.empty() == rule_seq.empty();

    const auto &rule = rules[rule_seq.front()];

    if (rule.type == RULE_LITERAL) {
        if (s.front() == rule.alternatives[0].front()) {
            small_vector<int> new_ruleset;
            new_ruleset.append_range(std::span(rule_seq).subspan(1));
            return match(s.substr(1), rules, new_ruleset);
        } else {
            return false;
        }
    } else {
        for (const auto &alt : rule.alternatives) {
            if (!alt.empty()) {
                small_vector<int> new_ruleset;
                new_ruleset.append_range(alt);
                new_ruleset.append_range(std::span(rule_seq).subspan(1));
                if (match(s, rules, new_ruleset))
                    return true;
            }
        }
    }

    return false;
}

static bool match(std::string_view s, std::span<const Rule> rules)
{
    small_vector<int> initial_rule_seq{0};
    return match(s, rules, initial_rule_seq);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto separator = std::ranges::find(lines, "");
    ASSERT(separator != lines.end());

    small_vector<Rule, 256> rules(separator - lines.begin());

    for (auto it = lines.begin(); it != separator; ++it) {
        auto [i, rule] = parse_rule(*it);
        rules[i] = rule;
    }

    // Part 1:
    {
        int matching_strings = 0;
        for (auto it = separator + 1; it != lines.end(); ++it)
            matching_strings += match(*it, rules);
        fmt::print("{}\n", matching_strings);
    }

    // Part 2:
    {
        rules[8] = Rule{RULE_ALT, {{42}, {42, 8}}};
        rules[11] = Rule{RULE_ALT, {{42, 31}, {42, 11, 31}}};

        int matching_strings = 0;
        for (auto it = separator + 1; it != lines.end(); ++it)
            matching_strings += match(*it, rules);
        fmt::print("{}\n", matching_strings);
    }
}

}
