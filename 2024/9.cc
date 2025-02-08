#include "common.h"
#include "dense_map.h"

namespace aoc_2024_9 {

static int64_t checksum(std::span<const int> disk)
{
    int64_t result = 0;

    for (size_t i = 0; i < disk.size(); ++i)
        if (disk[i] >= 0)
            result += i * disk[i];

    return result;
}

static std::vector<int>
build_disk(std::string_view line,
           std::vector<std::pair<uint32_t, uint32_t>> *files = nullptr)
{
    std::vector<int> disk;
    disk.reserve(10 * line.size());

    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] >= '0' && line[i] <= '9') {
            const int n = line[i] - '0';
            const int f = i % 2 == 0 ? i / 2 : -1;

            if (i % 2 == 0) {
                if (files)
                    files->emplace_back(disk.size(), n);
            }
            for (int j = 0; j < n; ++j)
                disk.push_back(f);
        }
    }

    return disk;
}

static int64_t part1(std::string_view line)
{
    auto disk = build_disk(line);

    for (size_t i = 0, j = disk.size() - 1;;) {
        while (disk[i] >= 0)
            i++;
        while (disk[j] < 0)
            j--;
        if (i >= j)
            break;
        disk[i] = disk[j];
        disk[j] = -1;
        j--;
    }

    return checksum(disk);
}

static int64_t part2(std::string_view line)
{
    std::vector<std::pair<uint32_t, uint32_t>> files;
    auto disk = build_disk(line, &files);

    for (size_t id = files.size(); id--;) {
        auto [file_pos, file_len] = files[id];
        size_t i = 0;

        while (true) {
            while (i < file_pos && disk[i] >= 0)
                i++;
            if (i >= file_pos || i >= disk.size())
                goto skip;

            auto j = i + 1;
            while (disk[j] < 0)
                j++;
            if (j - i >= file_len)
                break;

            i = j + 1;
        }

        std::ranges::fill(disk.begin() + i, disk.begin() + i + file_len, id);
        std::ranges::fill(disk.begin() + file_pos, disk.begin() + file_pos + file_len,
                          -1);

    skip:;
    }

    return checksum(disk);
}

void run(FILE *f)
{
    auto buf = slurp(f);
    fmt::print("{}\n", part1(buf));
    fmt::print("{}\n", part2(buf));
}

}
