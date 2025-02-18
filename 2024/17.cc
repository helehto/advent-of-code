#include "common.h"

namespace aoc_2024_17 {

static int64_t step(const int64_t a, std::span<const int> prog)
{
    enum { A, B, C };
    int64_t r[] = {a, 0, 0};

    size_t ip = 0;
    while (true) {
        const int op = prog[ip];
        const int val = prog[ip + 1];
        const int64_t combo = val < 4 ? val : std::array{r[A], r[B], r[C]}[val - 4];
        if (op == 0)
            r[A] >>= combo;
        else if (op == 1)
            r[B] ^= val;
        else if (op == 2)
            r[B] = combo & 7;
        else if (op == 3)
            ip = r[A] ? val : ip - 2;
        else if (op == 4)
            r[B] ^= r[C];
        else if (op == 5)
            return combo & 7;
        else if (op == 6)
            r[B] = r[A] >> combo;
        else if (op == 7)
            r[C] = r[A] >> combo;
        else
            ASSERT(false);
        ip += 2;
    }
}

static bool search(std::span<const int> prog, const ssize_t i, const int64_t a = 0)
{
    if (i < 0) {
        fmt::print("\n{}\n", a);
        return true;
    }

    for (size_t k = 0; k < 8; ++k) {
        if (step(a << 3 | k, prog) == prog[i] && search(prog, i - 1, a << 3 | k))
            return true;
    }

    return false;
}

void run(std::string_view buf)
{
    std::vector<int> nums;
    find_numbers(buf, nums);
    std::span<const int> prog(nums.begin() + 3, nums.end());

    int64_t a = nums[0];
    for (const char *sep = ""; a > 0; a >>= 3, sep = ",")
        fmt::print("{}{}", sep, step(a, prog));
    search(prog, prog.size() - 1);
}

}
