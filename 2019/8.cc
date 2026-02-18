#include "common.h"
#include <charconv>
#include <hwy/highway.h>

namespace aoc_2019_8 {

constexpr size_t layer_cols = 25;
constexpr size_t layer_rows = 6;
constexpr size_t layer_size = layer_cols * layer_rows;

static char *part1(char *out, const uint8_t *input, size_t num_layers)
{
    // TODO: Don't assume that we have 256-bit vectors.
    using D = hn::FixedTag<uint8_t, 32>;
    constexpr D d;

    int min0 = INT_MAX;
    int product = 0;
    for (size_t i = 0; i < num_layers; i++) {
        // Each ASCII character gets 8 separate slots that are incremented in
        // the unrolled loop below. 8-byte integers are used to simplify
        // vectorization with no risk of overflow, as the input string is
        // guaranteed to be at most 25 * 6 = 150 characters.
        alignas(32) uint8_t histogram[256][8];

        // Zero-fill all histogram slots for the ASCII characters '0', '1' and
        // '2' (and also '3', but that does not matter in our case).
        hn::Store(hn::Zero(d), d, histogram['0']);

        size_t j = i * layer_size;
        // The core loop: build a histogram of all ASCII characters in the
        // input string. This is unrolled and stores to eight different slots
        // per iteration to reduce loop-carried dependencies.
        for (; j + 7 < (i + 1) * layer_size; j += 8) {
            histogram[input[j + 0]][0]++;
            histogram[input[j + 1]][1]++;
            histogram[input[j + 2]][2]++;
            histogram[input[j + 3]][3]++;
            histogram[input[j + 4]][4]++;
            histogram[input[j + 5]][5]++;
            histogram[input[j + 6]][6]++;
            histogram[input[j + 7]][7]++;
        }
        for (; j < (i + 1) * layer_size; j++)
            histogram[input[j]][0]++;

        const hn::Vec<D> counts = hn::Load(d, histogram['0']);
        alignas(32) uint64_t freq[4];
        hn::Store(hn::SumsOf8(counts), hn::FixedTag<uint64_t, 4>(), freq);

        if (min0 > static_cast<int64_t>(freq[0])) {
            min0 = freq[0];
            product = freq[1] * freq[2];
        }
    }

    out = std::to_chars(out, out + 4096, product).ptr;
    *out++ = '\n';
    return out;
}

static char *part2(char *out, const char *input, size_t num_layers)
{
    alignas(64) std::array<uint8_t, layer_size> image;
    image.fill('2');
    std::string_view sv(input, num_layers * layer_size);

    for (size_t i = 0; i < num_layers; i++) {
        const auto *layer = reinterpret_cast<const uint8_t *>(input) + i * layer_size;
        for (size_t j = 0; j < layer_size; j++)
            image[j] = (image[j] == '2') ? layer[j] : image[j];
    }

    for (size_t i = 0; i < layer_rows; i++) {
        auto *p = image.data() + i * layer_cols;
        for (size_t i = 0; i < layer_cols; i++)
            *out++ = *p++ != '0' ? '#' : ' ';
        *out++ = '\n';
    }
    return out;
}

void run(std::string_view buf)
{
    size_t num_layers = buf.size() / layer_size;

    char output_buffer[4096];
    char *p = output_buffer;
    p = part1(p, reinterpret_cast<const uint8_t *>(buf.data()), num_layers);
    p = part2(p, buf.data(), num_layers);
    write(1, output_buffer, p - output_buffer);
}

}
