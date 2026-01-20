#include "common.h"
#include "thread_pool.h"

namespace aoc_2023_12 {

static uint64_t search(const char *s,
                       size_t n,
                       std::span<const uint32_t> blocks,
                       std::vector<uint64_t> &buffer)
{
    const size_t rows = blocks.size() + 1;
    const size_t cols = 2 * n;
    buffer.resize(rows * cols);
    MatrixView<uint64_t> m(buffer.data(), rows, cols);
    std::ranges::fill(m.all(), 0);

    std::vector<uint8_t> max_block_size(2 * m.cols, 0);

    // Compute the longest block that can placed at each position.
    {
        size_t i = 0;
        while (i < n && s[i] == '.')
            ++i;
        while (true) {
            size_t j = i;
            while (j < n && s[j] != '.')
                ++j;
            for (size_t k = i; k < j; ++k)
                max_block_size[k] = j - k;
            i = j;
            while (i < n && s[i] == '.')
                ++i;
            if (i >= n)
                break;
        }
    }

    size_t k = m.cols - 1;
    do {
        m(0, k) = 1;
    } while (k-- && (k >= n || s[k] != '#'));

    for (size_t i = 1; i <= blocks.size(); ++i) {
        for (size_t j = n; j--;) {
            auto b = blocks[blocks.size() - i];

            auto dot = m(i, j + 1);
            auto hash =
                max_block_size[j] >= b && s[j + b] != '#' ? m(i - 1, j + b + 1) : 0;

            if (s[j] == '.')
                m(i, j) = dot;
            else if (s[j] == '#')
                m(i, j) = hash;
            else
                m(i, j) = dot + hash;
        }
    }

    return m(m.rows - 1, 0);
}

void run(std::string_view buf)
{
    std::atomic<uint64_t> part1 = 0;
    std::atomic<uint64_t> part2 = 0;
    auto lines = split_lines(buf);

    ThreadPool::get().for_each_slice(lines, [&](std::span<const std::string_view> slice) {
        uint64_t local_part1 = 0;
        uint64_t local_part2 = 0;
        small_vector<uint32_t> n;
        small_vector<uint32_t> n2;
        std::vector<uint64_t> buffer;
        std::string s2;
        for (std::string_view sv : slice) {
            find_numbers(sv, n);
            std::string_view s = sv.substr(0, sv.find(' '));
            local_part1 += search(s.data(), s.size(), n, buffer);

            s2.clear();
            fmt::format_to(std::back_inserter(s2), "{0}?{0}?{0}?{0}?{0}", s);
            n2.clear();
            for (int i = 0; i < 5; ++i)
                n2.append_range(n);
            local_part2 += search(s2.data(), s2.size(), n2, buffer);
        }

        part1.fetch_add(local_part1, std::memory_order_relaxed);
        part2.fetch_add(local_part2, std::memory_order_relaxed);
    });

    fmt::print("{}\n{}\n", part1.load(), part2.load());
}

}
