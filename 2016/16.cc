#include "common.h"

namespace aoc_2016_16 {

void run(std::string_view buf)
{
    std::string disk;
    disk.reserve(2 * 35651584 + 1);

    std::string checksum;
    std::string next;

    auto solve = [&](size_t n) {
        disk.clear();
        disk.insert(disk.end(), buf.begin(), buf.end());

        while (disk.size() < n) {
            size_t i = disk.size();
            disk.push_back('0');
            for (size_t j = i; j--;)
                disk.push_back(disk[j] ^ 1);
        }
        disk.resize(n);

        checksum = disk;
        do {
            next.clear();
            for (size_t i = 0; i < checksum.size(); i += 2)
                next.push_back(checksum[i] == checksum[i + 1] ? '1' : '0');
            checksum.swap(next);
        } while (checksum.size() % 2 == 0);

        return checksum;
    };

    fmt::print("{}\n", solve(272));
    fmt::print("{}\n", solve(35651584));
}

}
