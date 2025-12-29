#include "dense_map.h"
#include "small_vector.h"
#include "thread_pool.h"
#include <atomic>

struct alignas(16) WorkPackage {
    int counter;
    char padding[12];
};
static_assert(sizeof(WorkPackage) == 16);

const size_t n_threads = std::thread::hardware_concurrency();
const size_t stride = 4;
const int64_t target = 10'000'000;

static std::atomic<int64_t> total_sum;
static std::atomic<int64_t> total_work_items_processed;

int main()
{
    auto &pool = ThreadPool::get();
    pool.start(n_threads);

    ForkPool<WorkPackage> fork_pool(pool.num_threads());
    WorkPackage initial{.counter = 0};
    fork_pool.push(std::span(&initial, 1));

    std::atomic<int64_t> total_sum = 0;
    fork_pool.run(
        pool, [&](const WorkPackage &work, small_vector_base<WorkPackage> &next_work) {
            total_sum.fetch_add(work.counter, std::memory_order_relaxed);
            if (work.counter % stride == 0)
                for (size_t i = 1; i <= stride; ++i)
                    if (const int next = work.counter + i; next < target)
                        next_work.emplace_back(next);
        });

    const int64_t expected = (target * (target - 1)) / 2;
    const int64_t actual = total_sum.load();
    ASSERT_MSG(actual == expected, "actual={} expected={}", actual, expected);
}
