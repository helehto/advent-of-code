#include "common.h"

namespace aoc_2021_20 {

void run(std::string_view buf)
{
    std::array<uint8_t, 512> pattern;

    auto nl = buf.find("\n\n");
    for (size_t i = 0, j = 0; i < nl; ++i) {
        if (buf[i] == '.' || buf[i] == '#')
            pattern[j++] = (buf[i] == '#');
    }

    auto image_lines = split_lines(buf.substr(nl + 2));
    constexpr int max_iterations = 50;
    constexpr int padding = max_iterations + 3;

    size_t x0 = padding;
    size_t x1 = padding + image_lines[0].size();
    size_t y0 = padding;
    size_t y1 = padding + image_lines.size();

    Matrix<uint8_t> image(y1 + padding, x1 + padding + 2, 0);
    Matrix<uint8_t> new_image(y1 + padding, x1 + padding + 2, 0);

    for (size_t y = y0; y < y1; ++y)
        for (size_t x = x0; x < x1; ++x)
            image(y, x) = (image_lines[y - y0][x - x0] == '#');

    bool outside_is_lit = false;
    for (size_t n = 0; n < max_iterations; ++n, outside_is_lit = !outside_is_lit) {
        const int pad = pattern[0] && !outside_is_lit;

        for (size_t y = y0 - 3; y < y1 + 3; ++y) {
            new_image(y, x0 - 3) = pad;
            new_image(y, x1 + 2) = pad;
        }
        for (size_t x = x0 - 3; x < x1 + 3; ++x) {
            new_image(y0 - 3, x) = pad;
            new_image(y1 + 2, x) = pad;
        }

        for (size_t y = y0 - 2; y < y1 + 2; ++y) {
            new_image(y, x0 - 2) = pad;
            new_image(y, x1 + 1) = pad;
        }
        for (size_t x = x0 - 2; x < x1 + 2; ++x) {
            new_image(y0 - 2, x) = pad;
            new_image(y1 + 1, x) = pad;
        }

        for (size_t y = y0 - 1; y < y1 + 1; ++y) {
            for (size_t x = x0 - 1; x < x1 + 1; ++x) {
                int k = 0;

                k |= image(y - 1, x - 1) << 8;
                k |= image(y - 1, x + 0) << 7;
                k |= image(y - 1, x + 1) << 6;
                k |= image(y + 0, x - 1) << 5;
                k |= image(y + 0, x + 0) << 4;
                k |= image(y + 0, x + 1) << 3;
                k |= image(y + 1, x - 1) << 2;
                k |= image(y + 1, x + 0) << 1;
                k |= image(y + 1, x + 1) << 0;

                new_image(y, x) = pattern[k];
            }
        }

        y0--;
        y1++;
        x0--;
        x1++;
        std::swap(image, new_image);

        if (n == 1 || n == max_iterations - 1)
            fmt::print("{}\n", std::ranges::count(image.all(), 1));
    }
}

}
