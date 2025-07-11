#include "common.h"
#include "md5.h"
#include <tuple>
#include <vector>

namespace aoc_2016_14 {

struct InterestingHash {
    uint32_t index;
    std::optional<char> x3;
    std::optional<char> x5;
    std::string hash;
};

static std::optional<char> check_x3(const char *h)
{
    for (size_t j = 2; j < 32; ++j)
        if (h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return std::nullopt;
}

static std::optional<char> check_x5(const char *h)
{
    for (size_t j = 4; j < 32; ++j)
        if (h[j - 4] == h[j] && h[j - 3] == h[j] && h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return std::nullopt;
}

static void solve1(std::string_view prefix)
{
    md5::State md5(prefix);
    std::vector<InterestingHash> ih;

    uint32_t n = 0;
    auto expand1 = [&] {
        auto hex = md5.run(n).to_hex();

        for (int i = 0; i < 8; ++i, ++n) {
            char *h = hex[i].data();
            auto tc = check_x3(h);
            auto qc = check_x5(h);

            if (tc || qc) {
                ih.push_back(InterestingHash{
                    .index = n,
                    .x3 = tc,
                    .x5 = qc,
                    .hash = std::string(std::string_view(h, 32)),
                });
            }
        }
    };

    while (ih.empty())
        expand1();

    size_t keys_found = 0;
    for (size_t i = 0;; i++) {
        while (ih.size() <= i || ih.back().index <= ih[i].index + 1000)
            expand1();

        for (size_t j = i + 1; j < ih.size() && ih[j].index <= ih[i].index + 1000; j++) {
            if (ih[j].x5 && *ih[i].x3 == *ih[j].x5) {
                keys_found++;
                if (keys_found == 64) {
                    fmt::print("{}\n", ih[i].index);
                    return;
                }
            }
        }
    }
}

static std::array<std::array<char, 32>, 8>
md5_hex_stretch1(const std::array<std::array<char, 32>, 8> &hex)
{
    md5::Block32x16x8 messages{};

    char *p = reinterpret_cast<char *>(messages.data);
    for (size_t w = 0; w < 8; ++w) {
        for (size_t i = 0; i < 8; ++i) {
            *p++ = hex[i][4 * w + 0];
            *p++ = hex[i][4 * w + 1];
            *p++ = hex[i][4 * w + 2];
            *p++ = hex[i][4 * w + 3];
        }
    }

    static constexpr uint32_t lengths[8] = {32, 32, 32, 32, 32, 32, 32, 32};
    md5::prepare_final_blocks(messages, lengths);

    return md5::do_block_avx2(messages).to_hex();
}

static void solve2(std::string_view prefix)
{
    md5::State md5(prefix);
    std::vector<InterestingHash> ih;
    uint32_t n = 0;

    auto expand1 = [&] {
        auto hex = md5.run(n).to_hex();
        for (int i = 0; i < 2016; ++i)
            hex = md5_hex_stretch1(hex);

        for (int i = 0; i < 8; ++i, ++n) {
            const char *h = hex[i].data();
            auto tc = check_x3(h);
            auto qc = check_x5(h);

            if (tc || qc) {
                ih.push_back(InterestingHash{
                    .index = n,
                    .x3 = tc,
                    .x5 = qc,
                    .hash = std::string(std::string_view(h, 32)),
                });
            }
        }
    };

    while (ih.empty())
        expand1();

    size_t keys_found = 0;
    for (size_t i = 0;; i++) {
        while (ih.size() <= i || ih.back().index <= ih[i].index + 1000)
            expand1();

        for (size_t j = i + 1; j < ih.size() && ih[j].index <= ih[i].index + 1000; j++) {
            if (ih[j].x5 && *ih[i].x3 == *ih[j].x5) {
                keys_found++;
                if (keys_found == 64) {
                    fmt::print("{}\n", ih[i].index);
                    return;
                }
            }
        }
    }
}

void run(std::string_view buf)
{
    solve1(buf);
    solve2(buf);
}

}
