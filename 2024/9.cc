#include "common.h"
#include "dense_map.h"

namespace aoc_2024_9 {

static int64_t part1(std::string_view line)
{
    size_t disk_size = 0;
    for (size_t i = 0; i < line.size(); ++i)
        disk_size += line[i] - '0';

    auto disk = std::make_unique_for_overwrite<int16_t[]>(disk_size + 16);
    size_t offset = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        const int n = line[i] - '0';
        const int f = i % 2 == 0 ? i / 2 : -1;
        _mm256_storeu_si256((__m256i *)&disk[offset], _mm256_set1_epi16(f));
        offset += n;
    }

    size_t i = 0;
    size_t j = disk_size - 1;
    while (true) {
        while (disk[i] >= 0)
            i++;
        while (disk[j] < 0)
            j--;
        if (i >= j)
            break;
        disk[i] = disk[j];
        i++;
        j--;
    }

    uint64_t checksum = 0;
    for (size_t k = 0; k <= j; ++k)
        if (disk[k] >= 0)
            checksum += k * disk[k];

    return checksum;
}

static int64_t part2(std::string_view line)
{
    std::vector<std::pair<uint32_t, uint32_t>> files;
    files.reserve(line.size() / 2);

    std::array<std::vector<uint32_t>, 10> gaps;
    for (size_t i = 0, offset = 0; i < line.size(); ++i) {
        const int n = line[i] - '0';
        if (i % 2 == 0)
            files.emplace_back(offset, n);
        else
            gaps[n].push_back(offset);

        offset += n;
    }

    for (std::vector<uint32_t> &g : gaps) {
        g.push_back(UINT32_MAX);
        std::ranges::make_heap(g, std::greater<>());
    }

    for (size_t id = files.size(); id--;) {
        auto &[file_pos, file_len] = files[id];

        size_t gap_pos = SIZE_MAX;
        size_t gap_len;
        for (size_t len = 9; len >= file_len; --len) {
            if (auto &g = gaps[len]; g.front() < gap_pos) {
                gap_pos = g.front();
                gap_len = len;
            }
        }

        if (gap_pos > file_pos)
            continue; // either no gap large enough, or would move file forward

        file_pos = gap_pos;
        std::ranges::pop_heap(gaps[gap_len], std::greater<>());
        gaps[gap_len].pop_back();

        if (int new_len = gap_len - file_len) {
            const int new_pos = gap_pos + file_len;
            gaps[new_len].push_back(new_pos);
            std::ranges::push_heap(gaps[new_len], std::greater<>());
        }
    }

    int64_t checksum = 0;
    for (size_t id = 0; id < files.size(); ++id) {
        const auto [pos, len] = files[id];
        checksum += id * len * (2 * pos + len - 1) / 2;
    }

    return checksum;
}

void run(std::string_view buf)
{
    fmt::print("{}\n", part1(buf));
    fmt::print("{}\n", part2(buf));
}

}
