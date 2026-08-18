#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/md5.h>
#include <aoc/small_vector.h>
#include <aoc/thread_pool.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <hwy/base.h>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

namespace aoc_2016_14 {

namespace hn = hwy::HWY_NAMESPACE;

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
    //
    // Note: using only bitwise operations and adds here lets GCC autovectorize
    // this across the loop iterations in to_hex(). It may seem tempting to
    // fold this into a single pext instruction on x86-64, but that makes the
    // entire loop scalar, which slows things down.
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

static std::array<std::array<char, 32>, md5::max_lanes> to_hex(md5::Vec4T r)
{
    std::array<uint32_t, md5::max_lanes> a, b, c, d;
    hn::Store(hn::Get4<0>(r), md5::D(), a.data());
    hn::Store(hn::Get4<1>(r), md5::D(), b.data());
    hn::Store(hn::Get4<2>(r), md5::D(), c.data());
    hn::Store(hn::Get4<3>(r), md5::D(), d.data());

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

/// Transpose a 4x4 matrix of 32-bit integers in memory.
static void transpose32_4x4(const void *HWY_RESTRICT srcv,
                            const size_t src_row_stride,
                            void *HWY_RESTRICT dstv,
                            const size_t dst_row_stride)
{
    using D = hn::FixedTag<uint32_t, 4>;
    using Wide = hn::RepartitionToWide<D>;
    constexpr D d;
    constexpr Wide w;

    auto *HWY_RESTRICT src = static_cast<const uint32_t *>(srcv);
    hn::Vec<D> I0 = hn::LoadU(d, src + 0 * src_row_stride);
    hn::Vec<D> I1 = hn::LoadU(d, src + 1 * src_row_stride);
    hn::Vec<D> I2 = hn::LoadU(d, src + 2 * src_row_stride);
    hn::Vec<D> I3 = hn::LoadU(d, src + 3 * src_row_stride);

    const hn::Vec<Wide> U0 = hn::ZipLower(w, I0, I1);
    const hn::Vec<Wide> U1 = hn::ZipLower(w, I2, I3);
    const hn::Vec<Wide> U2 = hn::ZipUpper(w, I0, I1);
    const hn::Vec<Wide> U3 = hn::ZipUpper(w, I2, I3);
    const hn::Vec<D> D0 = hn::BitCast(d, hn::InterleaveLower(w, U0, U1));
    const hn::Vec<D> D1 = hn::BitCast(d, hn::InterleaveUpper(w, U0, U1));
    const hn::Vec<D> D2 = hn::BitCast(d, hn::InterleaveLower(w, U2, U3));
    const hn::Vec<D> D3 = hn::BitCast(d, hn::InterleaveUpper(w, U2, U3));

    auto *HWY_RESTRICT dst = static_cast<uint32_t *>(dstv);
    hn::StoreU(D0, d, dst + 0 * dst_row_stride);
    hn::StoreU(D1, d, dst + 1 * dst_row_stride);
    hn::StoreU(D2, d, dst + 2 * dst_row_stride);
    hn::StoreU(D3, d, dst + 3 * dst_row_stride);
}

static std::array<std::array<char, 32>, md5::max_lanes>
md5_hex_stretch1(const std::array<std::array<char, 32>, md5::max_lanes> &hex)
{
    HWY_ALIGN_MAX md5::InterleavedBlocks messages;
    const size_t lanes = md5::lanes();

    // Transform the 32-byte output hex strings back into interleaved 4-byte
    // blocks of ASCII characters ready to be fed directly into MD5 again.
    auto *src = reinterpret_cast<const uint32_t *>(&hex[0][0]);
    auto *dst = reinterpret_cast<uint32_t *>(messages.data);
    for (size_t i = 0; i < hn::Blocks(md5::D()); ++i, src += 32, dst += 4) {
        // We could actually transpose 4 entire ASCII digests of 32 bytes at a
        // time with 256-bit vectors rather than splitting them into two parts.
        // That doesn't seem to be appreciably faster though, so just do it
        // with 128-bit vectors for compatibility.
        transpose32_4x4(src + 0, 8, dst + 0 * lanes, lanes);
        transpose32_4x4(src + 4, 8, dst + 4 * lanes, lanes);
    }

    // Insert 0x80 byte and length of of each message (256 bits).
    hn::Store(hn::Set(md5::D(), 0x80), md5::D(), messages.data[8]);
    hn::Store(hn::Set(md5::D(), 0x100), md5::D(), messages.data[14]);

    constexpr uint16_t non_zero_mask = 0b0100'0001'1111'1111;
    return to_hex(md5::hash_block<non_zero_mask>(messages));
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

void run(std::string_view buf, aoc::Answer &answer)
{
    answer.add(solve1(buf));
    answer.add(solve2(buf));
}
AOC_REGISTER_SOLVER(2016, 14, run);

}
