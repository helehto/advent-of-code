#include "common.h"

namespace aoc_2025_6 {

void run(std::string_view buf)
{
    // Find the byte offset for each row. (Use find() here to avoid looking at
    // a single character at time; my input is ~18 KiB.)
    small_vector<size_t> row_offset{0};
    for (size_t i = 0;;) {
        size_t j = buf.find('\n', i + 1);
        if (j == std::string_view::npos)
            break;
        row_offset.push_back(j + 1);
        i = j;
    }

    std::string_view operators(buf.begin() + row_offset.back(), buf.end());

    // Find the byte offset for each column, based on the positions of the
    // operators. (Use find_first_not_of() here for the same reason as above.)
    small_vector<size_t> column_offset;
    column_offset.reserve((row_offset[1] - row_offset[0]) / 2);
    for (size_t i = 0;;) {
        size_t j = operators.find_first_not_of(' ', i);
        if (j == std::string_view::npos)
            break;
        column_offset.push_back(j);
        i = j + 1;
    }

    const auto rows = row_offset.size() - 1;
    const auto cols = column_offset.size();

    // Find the width of each column.
    auto column_width = std::make_unique<size_t[]>(cols);
    for (size_t i = 0; i < cols - 1; ++i)
        column_width[i] = column_offset[i + 1] - column_offset[i] - 1;
    column_width[cols - 1] = operators.size() - column_offset.back();

    auto column_buffer = std::make_unique<int64_t[]>(rows);

    // Utility to read out a column of numbers for part 1.
    auto read_column1 = [&](size_t col) {
        for (size_t i = 0; i < rows; ++i) {
            int64_t n = 0;
            for (size_t k = 0; k < column_width[col]; ++k)
                if (char c = buf[column_offset[col] + row_offset[i] + k]; c != ' ')
                    n = n * 10 + c - '0';
            column_buffer[i] = n;
        }
        return std::span(column_buffer.get(), rows);
    };

    // Utility to read out a column of numbers for part 2.
    auto read_column2 = [&](size_t col) {
        for (size_t k = column_width[col]; k--;) {
            int64_t n = 0;
            for (size_t i = 0; i < rows; ++i)
                if (char c = buf[row_offset[i] + column_offset[col] + k]; c != ' ')
                    n = n * 10 + c - '0';
            column_buffer[column_width[col] - 1 - k] = n;
        }
        return std::span(column_buffer.get(), rows);
    };

    // Fold a column with the specified operator.
    auto fold = [&](std::span<const int64_t> range, char op) {
        return op == '+' ? std::ranges::fold_left(range, 0, λab(a + b))
                         : std::ranges::fold_left(range, 1, λab(a * b));
    };

    int64_t part1 = 0;
    for (size_t j = 0; j < cols; ++j) {
        auto op = operators[column_offset[j]];
        part1 += fold(read_column1(j), op);
    }

    int64_t part2 = 0;
    for (size_t j = 0; j < cols; ++j) {
        auto op = operators[column_offset[j]];
        part2 += fold(read_column2(j), op);
    }

    fmt::print("{}\n{}\n", part1, part2);
}
}
