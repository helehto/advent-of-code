#include "common.h"
#include "thread_pool.h"

namespace aoc_2018_5 {

[[gnu::noinline]]
static size_t react(char *scratch, std::string_view input)
{
    size_t i = 1;
    scratch[0] = '\0';

    for (char c : input) {
        size_t mask = ((scratch[i] ^ 0x20) == c) ? SIZE_MAX : 0;
        i += mask | 1; // ++i if no reaction, --i if reaction
        scratch[i] = mask ? scratch[i] : c;
    }

    return i - 1;
}

void run(std::string_view buf)
{
    while (!buf.empty() && buf.back() == '\n')
        buf.remove_suffix(1);

    {
        auto scratch = std::make_unique_for_overwrite<char[]>(buf.size() + 1);
        fmt::print("{}\n", react(scratch.get(), buf));
    }

    std::atomic<size_t> min = SIZE_MAX;
    ThreadPool &pool = ThreadPool::get();
    pool.for_each_index('a', 'z' + 1, [&](char begin, char end) {
        auto spliced = std::make_unique_for_overwrite<char[]>(buf.size());
        auto scratch = std::make_unique_for_overwrite<char[]>(buf.size() + 1);
        for (char c = begin; c < end; c++) {
            size_t spliced_len = 0;
            for (char k : buf) {
                spliced[spliced_len] = k;
                if ((k | 0x20) != c)
                    spliced_len++;
            }
            atomic_store_min(
                min, react(scratch.get(), std::string_view(spliced.get(), spliced_len)));
        }
    });

    fmt::print("{}\n", min.load());
}

}
