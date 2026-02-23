#include "bitmanip.h"
#include "common.h"
#include "dense_map.h"

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

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_map<uint64_t, uint64_t, CrcHasher> memory;
    memory.reserve(100'000);

    // Part 1:
    {
        uint64_t sum = 0;
        uint64_t maskx = 0;
        uint64_t mask1 = 0;
        for (std::string_view line : lines) {
            if (line.starts_with("mask")) {
                std::tie(mask1, maskx) = parse_mask_line(line);
            } else {
                auto [addr, value] = find_numbers_n<int, 2>(line);
                auto result = (maskx & value) | mask1;
                if (auto [it, ok] = memory.emplace(addr, result); !ok) {
                    sum -= it->second;
                    it->second = result;
                }
                sum += result;
            }
        }
        fmt::print("{}\n", sum);
    }

    // Part 2:
    memory.clear();
    {
        uint64_t sum = 0;
        uint64_t mask1 = 0;
        uint64_t maskx = 0;
        for (std::string_view line : lines) {
            if (line.starts_with("mask")) {
                std::tie(mask1, maskx) = parse_mask_line(line);
            } else {
                auto [raw_addr, value] = find_numbers_n<int, 2>(line);
                uint64_t addr = raw_addr | mask1;
                const auto popcount = std::popcount(maskx);
                const uint64_t max_addr = uint64_t(1) << popcount;
                for (uint64_t a = 0; a < max_addr; a++) {
                    auto result = (addr & ~maskx) | pdep_u64(a, maskx);
                    if (auto [it, ok] = memory.emplace(result, value); !ok) {
                        sum -= it->second;
                        it->second = value;
                    }
                    sum += value;
                }
            }
        }
        fmt::print("{}\n", sum);
    }
}

}
