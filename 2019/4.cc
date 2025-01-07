#include "common.h"

namespace aoc_2019_4 {

static void increment(char *pass)
{
    int i = 5;
    while (pass[i] == '9')
        i--;
    char fill = ++pass[i];
    for (int j = i + 1; j < 6; j++)
        pass[j] = fill;
}

static bool is_valid_1(char *pass)
{
    for (size_t i = 1; i < 6; i++) {
        if (pass[i - 1] == pass[i])
            return true;
    }
    return false;
}

static bool is_valid_2(char *pass)
{
    size_t i = 0;
    while (i < 6) {
        size_t j = i + 1;
        while (j < 6 && pass[j] == pass[i])
            j++;
        if (j - i == 2)
            return true;
        i = j;
    }
    return false;
}

static int cmp(const char *a, const char *b)
{
    for (int i = 0; i < 6; i++) {
        if (int d = a[0] - b[0]; d != 0)
            return d;
    }
    return 0;
}

void run(FILE *f)
{
    auto [buf, _] = slurp_lines(f);
    std::vector<int> bounds;
    find_numbers(buf, bounds);

    alignas(8) std::array<char, 8> pass{};
    memcpy(pass.data(), buf.data(), 6);

    size_t i = 0;
    for (; i < 5; i++) {
        if (pass[i] > pass[i + 1])
            break;
    }
    for (size_t j = i + 1; j < 6; j++)
        pass[j] = pass[i];

    int n1 = 0;
    int n2 = 0;
    while (cmp(pass.data(), buf.data() + 7) < 0) {
        n1 += is_valid_1(pass.data());
        n2 += is_valid_2(pass.data());
        increment(pass.data());
    }
    fmt::print("{}\n", n1);
    fmt::print("{}\n", n2);
}

}
