#include "common.h"

namespace aoc_2016_4 {

static int part1(const std::vector<std::string_view> &lines,
                 std::vector<std::pair<std::string_view, uint16_t>> *rooms)
{
    int s = 0;

    for (std::string_view line : lines) {
        auto bracket = line.find('[');
        auto name = line.substr(0, bracket);
        auto checksum = line.substr(bracket + 1, 5);

        std::array<std::pair<int8_t, int8_t>, 26> count{};
        for (int i = 0; i < 26; ++i)
            count[i].second = i;
        for (uint8_t c : name)
            if (c >= 'a' && c <= 'z')
                count[c - 'a'].first--;

        std::ranges::sort(count);

        bool ok = true;
        for (int i = 0; i < 5; ++i) {
            if (count[i].second + 'a' != checksum[i]) {
                ok = false;
                break;
            }
        }

        if (ok) {
            auto [sector_id] = find_numbers_n<int, 1>(name.substr(name.rfind('-') + 1));
            s += sector_id;
            rooms->emplace_back(name, sector_id);
        }
    }

    return s;
}

static int part2(const std::vector<std::pair<std::string_view, uint16_t>> &rooms)
{
    std::string s;

    for (auto [room, sector_id] : rooms) {
        s = room;
        for (char &c : s)
            if (c >= 'a' && c <= 'z')
                c = (c - 'a' + sector_id) % 26 + 'a';

        if (s.find("north") != std::string::npos)
            return sector_id;
    }

    __builtin_trap();
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::pair<std::string_view, uint16_t>> rooms;
    rooms.reserve(lines.size());

    fmt::print("{}\n", part1(lines, &rooms));
    fmt::print("{}\n", part2(rooms));
}

}
