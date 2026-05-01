#include "common.h"

namespace aoc_2023_3 {

struct PartNumber {
    uint16_t offset;
    uint16_t value;
};

constexpr int read_number(const char *p)
{
    int n = 0;
    do {
        n = 10 * n + *p - '0';
        p++;
    } while (*p >= '0' && *p <= '9');
    return n;
}

static inplace_vector<PartNumber, 8> surrounding_part_numbers(
    const char *grid, size_t base_offset, std::span<const ptrdiff_t> strides)
{
    inplace_vector<PartNumber, 8> result;

    for (const auto stride : strides) {
        if (const auto *c = grid + base_offset + stride; *c >= '0' && *c <= '9') {
            while (c[-1] >= '0' && c[-1] <= '9')
                c--;

            const uint16_t offset = c - grid;
            if (std::ranges::none_of(result, λa(offset == a.offset)))
                result.unchecked_emplace_back(offset, read_number(c));
        }
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, '.');

    const ptrdiff_t strides[] = {
        -static_cast<ptrdiff_t>(grid.cols) - 1, // UL
        -static_cast<ptrdiff_t>(grid.cols),     // U
        -static_cast<ptrdiff_t>(grid.cols) + 1, // UR
        -1,                                     // L
        +1,                                     // R
        +static_cast<ptrdiff_t>(grid.cols) - 1, // DL
        +static_cast<ptrdiff_t>(grid.cols),     // D
        +static_cast<ptrdiff_t>(grid.cols) + 1, // DR
    };

    int sum=0;
    int ratio_sum = 0;
    for (size_t i = 0; i < grid.size(); i++) {
        char c = grid.data()[i];
        if (!(c >= '0' && c <= '9') && c != '.') {
            auto parts = surrounding_part_numbers(grid.data(), i, strides);
            for (const auto &part : parts)
                sum += part.value;
            if (c == '*' && parts.size() == 2)
                ratio_sum += parts[0].value * parts[1].value;
        }
    }

    fmt::print("{}\n{}\n", sum, ratio_sum);
}

}
