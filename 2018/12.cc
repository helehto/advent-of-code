#include "common.h"

namespace aoc_2018_12 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<uint8_t> state;
    for (char c : lines[0].substr(lines[0].find(':') + 2))
        state.push_back(c);

    uint32_t rules = 0;
    for (size_t i = 2; i < lines.size(); i++) {
        if (lines[i].ends_with('#')) {
            uint32_t v = 0;
            for (int j = 0; j < 5; ++j)
                if (lines[i][j] == '#')
                    v |= 1 << j;
            rules |= UINT64_C(1) << v;
        }
    }

    std::vector<uint8_t> new_state;

    static auto step = [&](int offset) {
        new_state.clear();
        new_state.resize(state.size() + 4, '.');

        // Compute new state.
        for (size_t j = 0; j < new_state.size(); j++) {
            int v = 0;
            for (int k = -2; k <= 2; k++) {
                auto old_index = static_cast<size_t>(j + k - 2);
                if (old_index < state.size() && state[old_index] == '#')
                    v |= 1 << (k + 2);
            }
            if (rules & (UINT64_C(1) << v))
                new_state[j] = '#';
        }

        // Trim back of the new state vector.
        while (!new_state.empty() && new_state.back() == '.')
            new_state.pop_back();

        // Trim front of the new state vector.
        ssize_t j = 0;
        for (; new_state[j] == '.'; j++)
            ;
        new_state.erase(new_state.begin(), new_state.begin() + j);

        offset += j - 2;
        state.swap(new_state);
        return offset;
    };

    auto index_sum = [&](std::span<const uint8_t> state, int64_t offset) {
        int64_t sum = 0;
        for (size_t i = 0; i < state.size(); ++i) {
            if (state[i] == '#')
                sum += static_cast<int64_t>(i) + offset;
        }
        return sum;
    };

    int offset = 0;
    int generation = 0;
    for (; generation < 20; generation++)
        offset = step(offset);
    fmt::print("{}\n", index_sum(state, offset));

    int old_offset = 0;
    for (; !std::ranges::equal(state, new_state); generation++)
        old_offset = std::exchange(offset, step(offset));
    int offset_per_step = offset - old_offset;

    constexpr uint64_t target = UINT64_C(50'000'000'000);
    uint64_t final_offset = offset + offset_per_step * (target - generation);
    fmt::print("{}\n", index_sum(state, final_offset));
}
}
