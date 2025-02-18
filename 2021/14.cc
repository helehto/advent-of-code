#include "common.h"
#include "dense_map.h"

namespace aoc_2021_14 {

auto increment_count(dense_map<std::string, int> &map, const std::string &key)
{
    if (auto [it, inserted] = map.emplace(key, 1); !inserted)
        it->second++;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::string_view tpl = lines[0];

    dense_map<std::string, std::string> rules;
    for (size_t i = 2; i < lines.size(); ++i) {
        auto arrow = lines[i].find(" -> ");
        ASSERT(arrow != std::string_view::npos);
        rules.emplace(std::string(lines[i].substr(0, arrow)),
                      std::string(lines[i].substr(arrow + 4)));
    }

    dense_map<std::string, uint64_t> elements;
    dense_map<std::string, uint64_t> bigrams;
    for (size_t i = 1; i < tpl.size(); i++) {
        elements[std::string(tpl.data() + i - 1, tpl.data() + i)]++;
        bigrams[std::string(tpl.data() + i - 1, tpl.data() + i + 1)]++;
    }

    for (int i = 0; i < 40; ++i) {
        dense_map<std::string, uint64_t> new_bigrams;

        for (auto &[bigram, count] : bigrams) {
            auto &t = rules[bigram];
            new_bigrams[bigram.substr(0, 1) + t] += count;
            new_bigrams[t + bigram.substr(1, 2)] += count;
            elements[t] += count;
        }

        if (i == 9 || i == 39) {
            uint64_t min = UINT64_MAX;
            uint64_t max = 0;
            for (auto &[_, count] : elements) {
                min = std::min(min, count);
                max = std::max(max, count);
            }

            fmt::print("{}\n", max - min + 1);
        }

        bigrams.swap(new_bigrams);
    }
}

}
