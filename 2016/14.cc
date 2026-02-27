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
constexpr char check_x3(std::span<const char, 32> h)
{
    for (size_t j = 2; j < 32; ++j)
        if (h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return '\0';
}

/// Check for consecutive quintuples in a 32-character hash.
constexpr char check_x5(std::span<const char, 32> h)
{
    for (size_t j = 4; j < 32; ++j)
        if (h[j - 4] == h[j] && h[j - 3] == h[j] && h[j - 2] == h[j] && h[j - 1] == h[j])
            return h[j];
    return '\0';
}

/// Format a 32-bit integer as 8 hex digits.
static void format_u32_hex(char *out, const uint32_t h)
{
    uint64_t v = h;

    // Swap the nibbles. (To see why, note that endianness affects the byte
    // order, not the order of nibbles within each byte; e.g. for 0x4d we want
    // to output '4' followed by 'd', i.e. the *second* nibble first.)
    v = ((v & 0x0f0f0f0f) << 4) | ((v & 0xf0f0f0f0) >> 4);

    // Expand each nibble to a full byte in the range 0x00-0x0f.
    v = ((v & 0xffff0000) << 16) | (v & 0x0000ffff);
    v = ((v & 0x0000ff00'0000ff00) << 8) | (v & 0x000000ff'000000ff);
    v = ((v & 0x00f000f0'00f000f0) << 4) | (v & 0x000f000f'000f000f);

    // Add 0x30 (ASCII '0') to each byte.
    const uint64_t ascii_digits0 = v + 0x30303030'30303030;

    // Nibbles between 0-9 are correct, but a-f need to be fixed up. Identify
    // these by adding 6, which causes them to carry into the next nibble.
    const uint64_t carries4 = v + 0x06060606'06060606;
    const uint64_t carries0 = (carries4 & 0x10101010'10101010) >> 4;

    // Add 0x27 (ASCII 'a' - 10 - 0x30) to the nibbles that need to be
    // fixed up, i.e. the ones that carried.
    const uint64_t ascii_hex_digits = ascii_digits0 + carries0 * 0x27;

    memcpy(out, &ascii_hex_digits, sizeof(ascii_hex_digits));
}

static std::array<std::array<char, 32>, md5::max_lanes> to_hex(const md5::Result &r)
{
    const std::array<uint32_t, md5::max_lanes> a = md5::Result::to_array(r.a());
    const std::array<uint32_t, md5::max_lanes> b = md5::Result::to_array(r.b());
    const std::array<uint32_t, md5::max_lanes> c = md5::Result::to_array(r.c());
    const std::array<uint32_t, md5::max_lanes> d = md5::Result::to_array(r.d());

    std::array<std::array<char, 32>, md5::max_lanes> result;
    for (size_t i = 0; i < md5::lanes(); i++) {
        char *p = result[i].data();
        format_u32_hex(p, a[i]);
        format_u32_hex(p + 8, b[i]);
        format_u32_hex(p + 16, c[i]);
        format_u32_hex(p + 24, d[i]);
    }

    return result;
}

static int solve1(std::string_view prefix)
{
    md5::State md5(prefix);
    std::vector<InterestingHash> ih;

    uint32_t n = 0;
    auto expand1 = [&] {
        auto hex = to_hex(md5.run(n));
        for (size_t i = 0; i < md5::lanes(); ++i, ++n)
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

/// Transpose four 4x4 blocks of 32-bit integers stored in SIMD vectors, i.e.
/// from this:
///
///     I0 = [a0 a1 a2 a3 | a4 a5 a6 a7 | ...]
///     I1 = [b0 b1 b2 b3 | b4 b5 b6 b7 | ...]
///     I2 = [c0 c1 c2 c3 | c4 c5 c6 c7 | ...]
///     I3 = [d0 d1 d2 d3 | d4 d5 d6 d7 | ...]
///
/// to this:
///
///     I0 = [a0 b0 c0 d0 | a4 b4 c4 d4 | ...]
///     I1 = [a1 b1 c1 d1 | a5 b5 c5 d5 | ...]
///     I2 = [a2 b2 c2 d2 | a6 b6 c6 d6 | ...]
///     I3 = [a3 b3 c3 d3 | a7 b7 c7 d7 | ...]
template <typename D>
static void
transpose_4x4(D d, hn::Vec<D> &I0, hn::Vec<D> &I1, hn::Vec<D> &I2, hn::Vec<D> &I3)
    requires(std::same_as<hn::TFromD<D>, uint32_t>)
{
    using W = hn::RepartitionToWide<D>;
    const hn::Vec<W> U0 = hn::BitCast(W(), hn::InterleaveLower(d, I0, I1));
    const hn::Vec<W> U1 = hn::BitCast(W(), hn::InterleaveLower(d, I2, I3));
    const hn::Vec<W> U2 = hn::BitCast(W(), hn::InterleaveUpper(d, I0, I1));
    const hn::Vec<W> U3 = hn::BitCast(W(), hn::InterleaveUpper(d, I2, I3));
    I0 = hn::BitCast(d, hn::InterleaveLower(W(), U0, U1));
    I1 = hn::BitCast(d, hn::InterleaveUpper(W(), U0, U1));
    I2 = hn::BitCast(d, hn::InterleaveLower(W(), U2, U3));
    I3 = hn::BitCast(d, hn::InterleaveUpper(W(), U2, U3));
}

static std::array<std::array<char, 32>, md5::max_lanes>
md5_hex_stretch1(const std::array<std::array<char, 32>, md5::max_lanes> &hex)
{
    HWY_ALIGN_MAX md5::InterleavedBlocks messages;
    const size_t lanes = md5::lanes();

    using D8 = hn::FixedTag<uint32_t, 8>;
    using B = hn::BlockDFromD<D8>;

    // TODO: This assumes that we have 256-bit vectors.
    auto transpose_4x4_2x = [&](size_t src, size_t dst1, size_t dst2) {
        auto *p = reinterpret_cast<const uint32_t *>(&hex[0][0]) + src;
        hn::Vec<D8> v0 = hn::LoadU(D8(), &p[0]);
        hn::Vec<D8> v1 = hn::LoadU(D8(), &p[8]);
        hn::Vec<D8> v2 = hn::LoadU(D8(), &p[16]);
        hn::Vec<D8> v3 = hn::LoadU(D8(), &p[24]);
        transpose_4x4(D8(), v0, v1, v2, v3);

        hn::Store(hn::ExtractBlock<0>(v0), B(), &messages.data[0 * lanes + dst1]);
        hn::Store(hn::ExtractBlock<0>(v1), B(), &messages.data[1 * lanes + dst1]);
        hn::Store(hn::ExtractBlock<0>(v2), B(), &messages.data[2 * lanes + dst1]);
        hn::Store(hn::ExtractBlock<0>(v3), B(), &messages.data[3 * lanes + dst1]);
        hn::Store(hn::ExtractBlock<1>(v0), B(), &messages.data[0 * lanes + dst2]);
        hn::Store(hn::ExtractBlock<1>(v1), B(), &messages.data[1 * lanes + dst2]);
        hn::Store(hn::ExtractBlock<1>(v2), B(), &messages.data[2 * lanes + dst2]);
        hn::Store(hn::ExtractBlock<1>(v3), B(), &messages.data[3 * lanes + dst2]);
    };

    // Transform the 32-byte output hex strings back into interleaved 4-word
    // blocks of ASCII characters ready to be fed into MD5 again.
    for (size_t i = 0; i < hn::Blocks(md5::D()); ++i)
        transpose_4x4_2x(4 * i * 8, 4 * i, 4 * i + 4 * lanes);

    // Insert 0x80 byte to mark the end of each message.
    hn::Store(hn::Set(md5::D(), 0x80), md5::D(), &messages.data[8 * lanes]);

    // Insert zero-padding and the length of each message (256 bits).
    const auto zero = hn::Zero(md5::D());
    hn::Store(zero, md5::D(), &messages.data[9 * lanes]);
    hn::Store(zero, md5::D(), &messages.data[10 * lanes]);
    hn::Store(zero, md5::D(), &messages.data[11 * lanes]);
    hn::Store(zero, md5::D(), &messages.data[12 * lanes]);
    hn::Store(zero, md5::D(), &messages.data[13 * lanes]);
    hn::Store(hn::Set(md5::D(), 0x100), md5::D(), &messages.data[14 * lanes]);
    hn::Store(zero, md5::D(), &messages.data[15 * lanes]);

    return to_hex(md5::hash_block(messages));
}

static int solve2(std::string_view prefix)
{
    ThreadPool &pool = ThreadPool::get();
    std::vector<InterestingHash> hashes;
    std::mutex hashes_mutex;
    const size_t stride = pool.num_threads() * md5::lanes();

    pool.for_each_thread([&](size_t thread_id) {
        md5::State md5(prefix);
        small_vector<InterestingHash, 128> local_hashes;

        // TODO: Hard-coded limit :(
        for (uint32_t n = md5::lanes() * thread_id; n < 30'000; n += stride) {
            auto hex = to_hex(md5.run(n));
            for (int i = 0; i < 2016; ++i)
                hex = md5_hex_stretch1(hex);

            for (size_t i = 0; i < md5::lanes(); ++i)
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
