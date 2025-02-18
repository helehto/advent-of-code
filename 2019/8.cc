#include "common.h"
#include <charconv>

namespace aoc_2019_8 {

constexpr size_t layer_cols = 25;
constexpr size_t layer_rows = 6;
constexpr size_t layer_size = layer_cols * layer_rows;

static char *part1(char *out, const uint8_t *input, size_t num_layers)
{
    const __m256i vzero = _mm256_setzero_si256();

    int min0 = INT_MAX;
    int product = 0;
    for (size_t i = 0; i < num_layers; i++) {
        // Each ASCII character gets 8 separate slots that are incremented in
        // the unrolled loop below. 8-byte integers are used to simplify
        // vectorization with no risk of overflow, as the input string is
        // guaranteed to be at most 25 * 6 = 150 characters.
        alignas(32) uint8_t histogram[256][8];
        __m256i *vhist0123 = reinterpret_cast<__m256i *>(&histogram['0']);

        // Zero-fill all histogram slots for the ASCII characters '0', '1' and
        // '2' (and also '3', but that does not matter in our case).
        _mm256_storeu_si256(vhist0123, vzero);

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

        // To sum the eight slots for the four characters and produce a final
        // count for each character, _mm256_sad_epu8 (vpsadbw) is used. This
        // instruction computes a sum of absolute differences of 8-byte blocks,
        // but with an all-zero subtrahend it effectively turns into a 4-way
        // horizontal sum of eight 8-bit integers, which is exactly what we
        // need.
        const __m256i vcounts = _mm256_loadu_si256(vhist0123);
        int64_t freq[4];
        const __m256i hsum = _mm256_sad_epu8(vcounts, vzero);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(freq), hsum);

        if (min0 > freq[0]) {
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
    char image[layer_size];
    memset(image, '2', layer_size);
    std::string_view sv(input, num_layers * layer_size);

    const __m256i vtwo = _mm256_set1_epi8('2');

    for (size_t i = 0; i < num_layers; i++) {
        const char *layer = input + i * layer_size;

        // Blend 32 characters at a time:
        size_t j = 0;
        for (; j + 31 < layer_size; j += 32) {
            const __m256i vimage =
                _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&image[j]));
            const __m256i vlayer =
                _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&layer[j]));
            const __m256i veq2 = _mm256_cmpeq_epi8(vimage, vtwo);
            const __m256i blended = _mm256_blendv_epi8(vimage, vlayer, veq2);
            _mm256_store_si256(reinterpret_cast<__m256i *>(&image[j]), blended);
        }
        for (; j < layer_size; j++)
            image[j] = (image[j] == '2') ? layer[j] : image[j];
    }

    for (size_t i = 0; i < layer_rows; i++) {
        char *p = image + i * layer_cols;
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
