#include "common.h"

struct Board {
    std::array<uint8_t, 25> nums;
    std::array<int8_t, 100> index;
    uint32_t mask = 0;
};

// clang-format off
static constexpr uint32_t win_masks[] = {
    0b0000000000000000000011111,
    0b0000000000000001111100000,
    0b0000000000111110000000000,
    0b0000011111000000000000000,
    0b1111100000000000000000000,
    0b0000100001000010000100001,
    0b0001000010000100001000010,
    0b0010000100001000010000100,
    0b0100001000010000100001000,
    0b1000010000100001000010000,
};
// clang-format on

static bool is_won(const Board &b)
{
    for (uint32_t mask : win_masks)
        if ((b.mask & mask) == mask)
            return true;

    return false;
}

static bool mark(Board &b, int n)
{
    if (auto i = b.index[n]; i >= 0) {
        b.mask |= 1 << i;
        return is_won(b);
    }
    return false;
}

static int unmarked_sum(const Board &b)
{
    int sum = 0;

    for (size_t i = 0; i < b.nums.size(); ++i)
        if ((b.mask & (1 << i)) == 0)
            sum += b.nums[i];

    return sum;
}

void run_2021_4(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    std::vector<int> called;
    find_numbers(lines[0], called);

    std::vector<Board> boards;
    std::vector<uint8_t> curr_board;
    std::vector<uint8_t> curr_line;
    for (size_t i = 1; i < lines.size(); i++) {
        std::string_view s = lines[i];
        if (!s.empty()) {
            curr_line.clear();
            find_numbers(s, curr_line);
            curr_board.insert(end(curr_board), begin(curr_line), end(curr_line));
            ASSERT(curr_board.size() <= 25);
            if (curr_board.size() == 25) {
                Board b;
                std::copy(begin(curr_board), end(curr_board), std::begin(b.nums));
                for (auto &i : b.index)
                    i = -1;
                for (size_t i = 0; i < 25; i++)
                    b.index[b.nums[i]] = i;
                boards.push_back(b);
                curr_board.clear();
            }
        }
    }

    for (int n : called) {
        for (size_t i = 0; i < boards.size();) {
            auto &b = boards[i];

            if (mark(b, n)) {
                if (boards.size() == 100) {
                    fmt::print("{}\n", n * unmarked_sum(b));
                } else if (boards.size() == 1) {
                    fmt::print("{}\n", n * unmarked_sum(boards[0]));
                    return;
                }
                boards[i] = boards.back();
                boards.pop_back();
            } else {
                i++;
            }
        }
    }
}
