#include "common.h"
#include "dense_map.h"

namespace aoc_2017_16 {

void run(std::string_view buf)
{
    std::string programs;
    programs.reserve(16);
    for (int i = 0; i < 16; ++i)
        programs += static_cast<char>('a' + i);

    std::vector<std::string> sequence;
    sequence.reserve(100);
    sequence.push_back(programs);

    std::vector<std::string_view> moves;
    moves.reserve(buf.size());
    split(buf, moves, ',');

    auto dance = [&] {
        for (std::string_view move : moves) {
            if (move[0] == 's') {
                auto [n] = find_numbers_n<int, 1>(move);
                std::ranges::rotate(programs, programs.begin() + programs.size() - n);
            } else if (move[0] == 'x') {
                auto [a, b] = find_numbers_n<int, 2>(move);
                std::swap(programs[a], programs[b]);
            } else if (move[0] == 'p') {
                auto f = [&](char c) { return c == move[1] || c == move[3]; };
                auto it1 = std::find_if(programs.begin(), programs.end(), f);
                auto it2 = std::find_if(std::next(it1), programs.end(), f);
                std::swap(*it1, *it2);
            }
        }
    };

    dance();
    fmt::print("{}\n", programs);

    dense_map<std::string, int> seen;
    seen.reserve(100);
    for (size_t i = 1; seen.emplace(programs, i).second; ++i) {
        sequence.push_back(programs);
        dance();
    }
    sequence.pop_back();
    fmt::print("{}\n", sequence[1'000'000'000 % sequence.size()]);
}

}
