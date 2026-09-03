#include <aoc/base.h>
#include <array>
#include <charconv>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <hwy/highway.h>
#include <string_view>

namespace aoc_2019_8 {

namespace hn = hwy::HWY_NAMESPACE;

constexpr size_t layer_cols = 25;
constexpr size_t layer_rows = 6;
constexpr size_t layer_size = layer_cols * layer_rows;

static std::string_view part1(char *out, const uint8_t *input, size_t num_layers)
{
    using D = hn::CappedTag<uint8_t, 32>;
    constexpr D d;
    static constexpr size_t histogram_slots = hn::MaxLanes(d) / 4;
    static_assert(histogram_slots == 4 || histogram_slots == 8);

    char *start = out;
    int min0 = INT_MAX;
    int product = 0;
    for (size_t i = 0; i < num_layers; i++) {
        // Each ASCII character gets 8 separate slots that are incremented in
        // the unrolled loop below. 8-byte integers are used to simplify
        // vectorization with no risk of overflow, as the input string is
        // guaranteed to be at most 25 * 6 = 150 characters.
        alignas(32) uint8_t histogram[256][histogram_slots];

        // Zero-fill all histogram slots for the ASCII characters '0', '1' and
        // '2' (and also '3', but that does not matter in our case).
        hn::Store(hn::Zero(d), d, histogram['0']);

        size_t j = i * layer_size;
        // The core loop: build a histogram of all ASCII characters in the
        // input string. This is unrolled and stores to different slots per
        // iteration to reduce loop-carried dependencies.
        for (; j + histogram_slots <= (i + 1) * layer_size; j += histogram_slots)
            for (size_t k = 0; k < histogram_slots; k++)
                histogram[input[j + k]][k]++;
        for (; j < (i + 1) * layer_size; j++)
            histogram[input[j]][0]++;

        const hn::Vec<D> counts = hn::Load(d, histogram['0']);
        if constexpr (histogram_slots == 8) {
            constexpr hn::Repartition<uint64_t, D> q;
            HWY_ALIGN_MAX uint64_t freq[hn::MaxLanes(q)];
            hn::Store(hn::SumsOf8(counts), q, freq);
            if (min0 > static_cast<int64_t>(freq[0])) {
                min0 = freq[0];
                product = freq[1] * freq[2];
            }
        } else {
            constexpr hn::Repartition<uint32_t, D> q;
            HWY_ALIGN_MAX uint32_t freq[hn::MaxLanes(q)];
            hn::Store(hn::SumsOf4(counts), q, freq);
            if (min0 > static_cast<int32_t>(freq[0])) {
                min0 = freq[0];
                product = freq[1] * freq[2];
            }
        }
    }

    out = std::to_chars(out, out + 4096, product).ptr;
    return std::string_view(start, out - start);
}

static std::string_view part2(char *out, const char *input, size_t num_layers)
{
    char *start = out;
    alignas(64) std::array<uint8_t, layer_size> image;
    image.fill('2');
    std::string_view sv(input, num_layers * layer_size);

    for (size_t i = 0; i < num_layers; i++) {
        const auto *layer = reinterpret_cast<const uint8_t *>(input) + i * layer_size;
        for (size_t j = 0; j < layer_size; j++)
            image[j] = (image[j] == '2') ? layer[j] : image[j];
    }

    for (size_t i = 0; i < layer_rows; i++) {
        if (i)
            *out++ = '\n';
        auto *p = image.data() + i * layer_cols;
        for (size_t i = 0; i < layer_cols; i++)
            *out++ = *p++ != '0' ? '#' : ' ';
    }

    return std::string_view(start, out - start);
}

void run(std::string_view buf, aoc::Answer &answer)
{
    size_t num_layers = buf.size() / layer_size;
    char out[4096];
    answer.add(part1(out, reinterpret_cast<const uint8_t *>(buf.data()), num_layers));
    answer.add(part2(out, buf.data(), num_layers));
}
AOC_REGISTER_SOLVER(2019, 8, run);

}
