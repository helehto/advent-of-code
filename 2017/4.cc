#include "common.h"
#include "dense_set.h"

namespace aoc_2017_4 {

void run(std::string_view buf)
{
    dense_set<uint64_t> words_ordered;
    words_ordered.reserve(16);
    dense_set<uint32_t> words_letters;
    words_letters.reserve(16);

    std::vector<std::string_view> words;
    words.reserve(16);

    int s1 = 0;
    int s2 = 0;
    for (std::string_view line : split_lines(buf)) {
        words_ordered.clear();
        words_letters.clear();

        for (std::string_view word : split(line, words, ' ')) {
            ASSERT(word.size() < 8);

            uint64_t word_ordered = 0;
            uint32_t word_letters = 0;
            for (char c : word) {
                word_ordered = word_ordered << 8 | (c - 'a');
                word_letters |= UINT32_C(1) << (c - 'a');
            }

            words_ordered.insert(word_ordered);
            words_letters.insert(word_letters);
        }
        s1 += words.size() == words_ordered.size();
        s2 += words.size() == words_letters.size();
    }
    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
