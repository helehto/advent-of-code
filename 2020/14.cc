#include "common.h"
#include "dense_map.h"
#include <bit>

namespace aoc_2020_14 {

static std::pair<uint64_t, uint64_t> parse_mask_line(std::string_view line)
{
    uint64_t maskx = 0;
    uint64_t mask1 = 0;
    for (char c : line.substr(7)) {
        maskx <<= 1;
        mask1 <<= 1;
        if (c == 'X') {
            maskx |= 1;
        } else if (c == '1') {
            mask1 |= 1;
        }
    }
    return std::pair(mask1, maskx);
}

static uint64_t memory_sum(const dense_map<uint64_t, uint64_t> &memory)
{
    uint64_t sum = 0;
    for (uint64_t x : std::ranges::views::values(memory))
        sum += x;
    return sum;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_map<uint64_t, uint64_t> memory;
    memory.reserve(100'000);

    // Part 1:
    {
        uint64_t maskx = 0;
        uint64_t mask1 = 0;
        for (std::string_view line : lines) {
            if (line.starts_with("mask")) {
                std::tie(mask1, maskx) = parse_mask_line(line);
            } else {
                auto [addr, value] = find_numbers_n<int, 2>(line);
                memory[addr] = (maskx & value) | mask1;
            }
        }
        fmt::print("{}\n", memory_sum(memory));
    }

    // Part 2:
    memory.clear();
    {
        uint64_t mask1 = 0;
        uint64_t maskx = 0;
        for (std::string_view line : lines) {
            if (line.starts_with("mask")) {
                std::tie(mask1, maskx) = parse_mask_line(line);
            } else {
                auto [raw_addr, value] = find_numbers_n<int, 2>(line);
                uint64_t addr = raw_addr | mask1;
                const uint64_t max_addr = uint64_t(1) << std::popcount(maskx);
                for (uint64_t a = 0; a < max_addr; a++)
                    memory[(addr & ~maskx) | _pdep_u64(a, maskx)] = value;
            }
        }
        fmt::print("{}\n", memory_sum(memory));
    }
}

}
