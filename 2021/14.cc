#include "common.h"
#include <fmt/core.h>
#include <unordered_map>

auto increment_count(std::unordered_map<std::string, int> &map, const std::string &key)
{
    if (auto [it, inserted] = map.emplace(key, 1); !inserted)
        it->second++;
}

void run_2021_14(FILE *f)
{
    std::string tpl;
    std::string s;
    getline(f, tpl);
    getline(f, s);

    std::unordered_map<std::string, std::string> rules;

    char a[16], b[16];
    while (getline(f, s)) {
        sscanf(s.c_str(), "%[^ ] -> %s\n", a, b);
        rules.emplace(a, b);
    }

    std::unordered_map<std::string, uint64_t> elements;
    std::unordered_map<std::string, uint64_t> bigrams;
    for (size_t i = 1; i < tpl.size(); i++) {
        elements[std::string(tpl.data() + i - 1, tpl.data() + i)]++;
        bigrams[std::string(tpl.data() + i - 1, tpl.data() + i + 1)]++;
    }

    for (int i = 0; i < 40; ++i) {
        std::unordered_map<std::string, uint64_t> new_bigrams;

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

            fmt::print("{}\n", max - min+1);
        }

        bigrams.swap(new_bigrams);
    }
}
