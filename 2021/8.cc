#include "common.h"
#include <array>

namespace aoc_2021_8 {

void run(std::string_view buf)
{
    std::string s;
    int part1 = 0;
    int part2 = 0;

    for (std::string_view line : split_lines(buf)) {
        s = line;

        const char *token;
        std::array<uint32_t, 14> masks = {{}};

        std::array<int8_t, 10> shuffle;
        shuffle.fill(-1);
        std::array<int8_t, 10> unshuffle;
        unshuffle.fill(-1);
        std::array<uint8_t, 128> mask2digit;

        auto resolve = [&](int shuffled, int unshuffled) {
            ASSERT(shuffle[unshuffled] == -1);
            shuffle[unshuffled] = shuffled;
            unshuffle[shuffled] = unshuffled;
            mask2digit[masks[shuffled]] = unshuffled;
        };

        token = strtok(s.data(), " |");
        for (size_t i = 0; i < masks.size(); i++) {
            for (const char *p = token; *p; p++)
                masks[i] |= 1 << (*p - 'a');
            token = strtok(NULL, " |");
        }

        for (int i = 10; i < 14; i++) {
            int c = std::popcount(masks[i]);
            if (c == 2 || c == 3 || c == 4 || c == 7)
                part1++;
        }

        // 1, 4, 7, 8
        for (int i = 0; i < 10; i++) {
            int c = std::popcount(masks[i]);
            if (c == 2)
                resolve(i, 1);
            else if (c == 3)
                resolve(i, 7);
            else if (c == 4)
                resolve(i, 4);
            else if (c == 7)
                resolve(i, 8);
        }

        // 2, 3
        for (int i = 0; i < 10; i++) {
            if (std::popcount(masks[i]) == 5) {
                if ((masks[shuffle[1]] & ~masks[i]) == 0)
                    resolve(i, 3);
                else if (std::popcount(masks[i] & ~masks[shuffle[4]]) == 3)
                    resolve(i, 2);
            }
        }

        // 9
        for (int i = 0; i < 10; i++) {
            if (std::popcount(masks[i]) == 6 &&
                std::popcount(masks[i] & ~masks[shuffle[3]]) == 1) {
                resolve(i, 9);
            }
        }

        // 0
        for (int i = 0; i < 10; i++) {
            if (std::popcount(masks[i]) == 6 &&
                std::popcount(masks[i] & ~masks[shuffle[9]]) == 1 &&
                (masks[shuffle[1]] & ~masks[i]) == 0) {
                resolve(i, 0);
            }
        }

        // 5, 6
        for (int i = 0; i < 10; i++) {
            if (unshuffle[i] != -1)
                continue;
            if (std::popcount(masks[i]) == 5)
                resolve(i, 5);
            else if (std::popcount(masks[i]) == 6)
                resolve(i, 6);
        }

        part2 += 1000 * mask2digit[masks[10]] + 100 * mask2digit[masks[11]] +
                 10 * mask2digit[masks[12]] + mask2digit[masks[13]];
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
