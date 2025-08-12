#include "common.h"

namespace aoc_2018_9 {

static const __m256i insert_shuffle_ctrl[] = {
    _mm256_setr_epi32(0, 0, 1, 2, 3, 4, 5, 6), _mm256_setr_epi32(0, 0, 1, 2, 3, 4, 5, 6),
    _mm256_setr_epi32(0, 1, 0, 2, 3, 4, 5, 6), _mm256_setr_epi32(0, 1, 2, 0, 3, 4, 5, 6),
    _mm256_setr_epi32(0, 1, 2, 3, 0, 4, 5, 6), _mm256_setr_epi32(0, 1, 2, 3, 4, 0, 5, 6),
    _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 0, 6), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 0),
};

static const __m256i pop_shuffle_ctrl[] = {
    _mm256_setr_epi32(1, 2, 3, 4, 5, 6, 7, 0), _mm256_setr_epi32(0, 2, 3, 4, 5, 6, 7, 0),
    _mm256_setr_epi32(0, 1, 3, 4, 5, 6, 7, 0), _mm256_setr_epi32(0, 1, 2, 4, 5, 6, 7, 0),
    _mm256_setr_epi32(0, 1, 2, 3, 5, 6, 7, 0), _mm256_setr_epi32(0, 1, 2, 3, 4, 6, 7, 0),
    _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 7, 0), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 0),
};

constexpr size_t block_capacity = 8;

/// Instead of tracking marbles individually using a linked list, we track them
/// in batches of max 8. This leads to better cache utilization due to having
/// more contiguous values and lower space overhead (due to having one 'next'
/// pointer per 8 elements instead of per each element).
///
/// The size 8 is specifically chosen to allow for quick insertion and deletion
/// of values from an arbitrary index in the array with the vpermd instruction
/// (_mm256_permutevar8x32_epi32) from AVX2.
struct MarbleBlock {
    static_assert(block_capacity == 8);

    std::array<int32_t, block_capacity> values;
    uint32_t n;

    // Since we allocate all blocks at once, they are contiguous. Shrink the
    // structure a little by encoding the link to the next block as a byte
    // offset relative to `this` stored as 4 bytes instead of a pointer (8
    // bytes). This assumes ≤ 100,000,000 marbles in part 2, but my input only
    // has around ~7,000,000.
    int32_t next_offset;

    void set_next(const MarbleBlock &other)
    {
        next_offset =
            reinterpret_cast<intptr_t>(&other) - reinterpret_cast<intptr_t>(this);
    }
    MarbleBlock &next()
    {
        const auto p = reinterpret_cast<intptr_t>(this);
        return *reinterpret_cast<MarbleBlock *>(p + next_offset);
    }

    /// Insert `value` into the given index in this block.
    void insert(const size_t index, const int32_t value)
    {
        const __m256i v = _mm256_loadu_si256((const __m256i *)&values);
        const __m256i perm = _mm256_permutevar8x32_epi32(v, insert_shuffle_ctrl[index]);
        _mm256_storeu_si256((__m256i *)&values, perm);
        values[index] = value;
        n++;
    }

    /// Insert `value` as the first element in this block.
    void push_front(const int32_t value)
    {
        const __m256i v = _mm256_loadu_si256((const __m256i *)&values);
        const __m256i perm = _mm256_permutevar8x32_epi32(v, insert_shuffle_ctrl[0]);
        const __m256i result = _mm256_insert_epi32(perm, value, 0);
        _mm256_storeu_si256((__m256i *)&values, result);
        n++;
    }

    /// Erase the marble at the given index in this block, returning it.
    int32_t erase(const size_t index)
    {
        const int32_t removed = values[index];
        const __m256i v = _mm256_loadu_si256((const __m256i *)&values);
        const __m256i perm = _mm256_permutevar8x32_epi32(v, pop_shuffle_ctrl[index]);
        _mm256_storeu_si256((__m256i *)&values, perm);
        n--;
        return removed;
    }
};

static int64_t play(MarbleBlock *blocks, int n_players, int n_marbles)
{
    MarbleBlock *curr = blocks;
    MarbleBlock *prev = curr;
    MarbleBlock *next = curr;
    curr->values = {0};
    curr->n = 1;
    curr->set_next(*curr);

    MarbleBlock *first_unused_block = blocks + 1;
    MarbleBlock *last_unlinked_block = nullptr;
    size_t block_offset = 0;
    std::vector<int64_t> scores(n_players);

    auto insert_new_block = [&](MarbleBlock &a, MarbleBlock &b) -> MarbleBlock * {
        MarbleBlock *block = last_unlinked_block;
        last_unlinked_block = nullptr;
        if (!block)
            block = first_unused_block++;

        block->n = 0;
        block->set_next(b);
        a.set_next(*block);
        return block;
    };

    auto add_marble = [&](int32_t i) {
        size_t new_marble_offset = block_offset + 2;

        if (new_marble_offset == curr->n + 1) {
            // The most common case: we are at the end of a non-full block.
            // Since we need to place the new marble two steps clockwise,
            // extract the first marble of the next block to place it just
            // before the new marble.
            const int m = next->erase(0);

            // If the next block is now empty, unhook it.
            if (next->n == 0) [[unlikely]] {
                curr->set_next(next->next());
                if (curr->next_offset != 0) {
                    // Mark this as the last freed block so that it can be
                    // reused by the next call to insert_new_block().
                    last_unlinked_block = next;
                    next = &curr->next();
                }
            }

            // If this block is full, insert a new block to place the new
            // marble in.
            if (curr->n >= block_capacity - 1) {
                prev = curr;
                curr = insert_new_block(*curr, *next);
                next = &curr->next();
            }

            curr->values[curr->n++] = m;
            curr->values[curr->n++] = i;
            block_offset = curr->n - 1;
            return;
        } else if (curr->n < block_capacity) {
            // If the new marble needs to be placed the middle of a non-full
            // block, we can just insert it directly without any additional
            // logic.
            block_offset = new_marble_offset;
            curr->insert(block_offset, i);
            return;
        }

        // The current block is full, so a marble will have to be moved into
        // the next block. Insert a new block inbetween if that one is full.
        if (next->n == block_capacity)
            next = insert_new_block(*curr, *next);

        if (new_marble_offset == block_capacity) {
            // Special case since insertions take two steps clockwise: if the
            // current index is next to last and we are full, we need to insert
            // the new marble as the first value into the next block.
            block_offset = 0;
            next->push_front(i);
            prev = curr;
            curr = next;
            next = &curr->next();
        } else {
            // The fallback and rarest case: the new marble needs to be placed
            // into the middle of the current block. Kick out the last marble
            // of this block into the next block to make space for it.
            curr->n--;
            next->push_front(curr->values[curr->n]);
            block_offset = new_marble_offset;
            curr->insert(block_offset, i);
        }
    };

    auto remove_marble = [&](int32_t i) {
        auto new_marble_offset = static_cast<ssize_t>(block_offset) - 7;
        if (new_marble_offset < 0) [[likely]] {
            // The marble to be removed is in the previous block. (This is the
            // common case; since we have 8 elements per block, the only way
            // the marble can be in the same block is if we are currently on
            // its last element, occurring only roughly 1/8 of the time.)
            next = curr;
            curr = prev;
            prev = nullptr;
            new_marble_offset += curr->n;
        }

        block_offset = new_marble_offset;
        int removed_marble = curr->erase(block_offset);
        scores[i % n_players] += i + removed_marble;
    };

    for (int i = 1; i <= n_marbles; i++) {
        if (i % 23 != 0)
            add_marble(i);
        else
            remove_marble(i);
    }

    return std::ranges::max(scores);
}

void run(std::string_view buf)
{
    auto [n_players, n_marbles] = find_numbers_n<int, 2>(buf);
    const auto num_blocks = (100 * n_marbles + 1 + 7) / 8;
    auto blocks = std::make_unique_for_overwrite<MarbleBlock[]>(num_blocks);
    fmt::print("{}\n", play(blocks.get(), n_players, n_marbles));
    fmt::print("{}\n", play(blocks.get(), n_players, 100 * n_marbles));
}

}
