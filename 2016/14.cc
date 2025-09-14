#include "common.h"
#include "md5.h"
#include "small_vector.h"
#include "thread_pool.h"

namespace aoc_2016_14 {

struct InterestingHash {
    uint32_t index;
    char x3;
    char x5;
};

/// Check for consecutive triples in a 32-character hash.
static char check_x3(std::span<const char, 32> h)
{
    for (size_t j = 2; j < 32; ++j)
        if (h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return '\0';
}

/// Check for consecutive quintuples in a 32-character hash.
static char check_x5(std::span<const char, 32> h)
{
    for (size_t j = 4; j < 32; ++j)
        if (h[j - 4] == h[j] && h[j - 3] == h[j] && h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return '\0';
}

static int solve1(std::string_view prefix)
{
    md5::State md5(prefix);
    std::vector<InterestingHash> ih;

    uint32_t n = 0;
    auto expand1 = [&] {
        auto hex = md5.run(n).to_hex();
        for (int i = 0; i < 8; ++i, ++n)
            if (auto x3 = check_x3(hex[i]), x5 = check_x5(hex[i]); x3 || x5)
                ih.emplace_back(n, x3, x5);
    };

    while (ih.empty())
        expand1();

    size_t keys_found = 0;
    for (size_t i = 0;; i++) {
        while (ih.size() <= i || ih.back().index <= ih[i].index + 1000)
            expand1();

        InterestingHash &a = ih[i];
        for (const InterestingHash &b : std::span(ih).subspan(i + 1)) {
            if (b.index > a.index + 1000)
                break;
            if (a.x3 == b.x5 && ++keys_found == 64)
                return a.index;
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

static int solve2(std::string_view prefix)
{
    ThreadPool &pool = ThreadPool::get();
    std::vector<InterestingHash> hashes;
    std::mutex hashes_mutex;

    pool.for_each_thread([&](size_t thread_id) {
        md5::State md5(prefix);
        small_vector<InterestingHash, 128> local_hashes;

        // TODO: Hard-coded limit :(
        for (uint32_t n = 8 * thread_id; n < 30'000; n += 8 * pool.num_threads()) {
            auto hex = md5.run(n).to_hex();
            for (int i = 0; i < 2016; ++i)
                hex = md5_hex_stretch1(hex);

            for (int i = 0; i < 8; ++i)
                if (char x3 = check_x3(hex[i]), x5 = check_x5(hex[i]); x3 || x5)
                    local_hashes.emplace_back(n + i, x3, x5);
        }

        std::unique_lock lock(hashes_mutex);
        hashes.append_range(local_hashes);
    });

    std::unique_lock lock(hashes_mutex);
    std::ranges::sort(hashes, {}, λa(a.index));

    size_t keys_found = 0;
    for (size_t i = 0; i < hashes.size(); ++i) {
        const InterestingHash &a = hashes[i];
        for (const InterestingHash &b : std::span(hashes).subspan(i + 1)) {
            if (b.index > a.index + 1000)
                break;
            if (a.x3 == b.x5 && ++keys_found == 64)
                return a.index;
        }
    }

    ASSERT_MSG(false, "No solution found!?");
}

void run(std::string_view buf)
{
    fmt::print("{}\n", solve1(buf));
    fmt::print("{}\n", solve2(buf));
}

}
