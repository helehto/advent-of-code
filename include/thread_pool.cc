#include "thread_pool.h"
#include <random>
#include <thread>

struct alignas(64) ThreadPool::Worker {
    std::jthread thread;
};

void futex_wake(const std::atomic_uint32_t &addr, int32_t n) noexcept
{
    if (syscall(SYS_futex, &addr, FUTEX_WAKE_PRIVATE, n) < 0)
        ASSERT_MSG(false, "futex(FUTEX_WAKE_PRIVATE) failed: {}", strerror(errno));
}

void futex_wake_bitset(const std::atomic_uint32_t &addr,
                       int32_t n,
                       uint32_t bitset) noexcept
{
    if (syscall(SYS_futex, &addr, FUTEX_WAKE_BITSET_PRIVATE, n, nullptr, nullptr,
                bitset) < 0)
        ASSERT_MSG(false, "futex(FUTEX_WAKE_BITSET_PRIVATE) failed: {}", strerror(errno));
}

bool futex_wait(const std::atomic_uint32_t &addr, uint32_t expected) noexcept
{
    if (syscall(SYS_futex, &addr, FUTEX_WAIT_PRIVATE, expected, nullptr) < 0) {
        ASSERT_MSG(errno == EAGAIN || errno == EINTR,
                   "futex(FUTEX_WAIT_PRIVATE) failed: {}", strerror(errno));
        return false;
    }

    return true;
}

bool futex_wait_bitset(const std::atomic_uint32_t &addr,
                       uint32_t expected,
                       uint32_t bitset) noexcept
{
    if (syscall(SYS_futex, &addr, FUTEX_WAIT_BITSET_PRIVATE, expected, nullptr, nullptr,
                bitset) < 0) {
        ASSERT_MSG(errno == EAGAIN || errno == EINTR,
                   "futex(FUTEX_WAIT_BITSET_PRIVATE) failed: {}", strerror(errno));
        return false;
    }

    return true;
}

void atomic_wait_zero(const std::atomic_uint32_t &counter,
                      std::memory_order order) noexcept
{
    DEBUG_ASSERT(order != std::memory_order_release &&
                 order != std::memory_order_acq_rel);
    while (true) {
        uint32_t val = counter.load(order);
        if (val == 0)
            break;
        futex_wait(counter, val);
    }
}

ThreadPool::ThreadPool() = default;

ThreadPool::~ThreadPool()
{
    state_.fetch_or(STATE_STOPPING, std::memory_order_seq_cst);
    futex_wake_bitset(state_, INT_MAX, STATE_STOPPING);
}

/// Acquire the thread pool lock.
void ThreadPool::lock() noexcept
{
    uint32_t old_state = state_.fetch_or(STATE_LOCKED, std::memory_order_acquire);

    while (old_state & STATE_LOCKED) [[unlikely]] {
        futex_wait_bitset(state_, old_state | STATE_LOCKED, STATE_LOCKED);
        old_state = state_.fetch_or(STATE_LOCKED, std::memory_order_acquire);
    }
}

/// Unlock the thread pool and mark the work queue as being non-empty.
void ThreadPool::unlock_with_work(size_t num_threads_to_wake) noexcept
{
    uint32_t old_state = state_.load(std::memory_order_relaxed);

    while (true) {
        auto new_state = old_state;
        new_state &= ~STATE_LOCKED;
        new_state |= STATE_HAS_WORK;
        if (state_.compare_exchange_weak(old_state, new_state, std::memory_order_release,
                                         std::memory_order_relaxed)) [[likely]] {
            futex_wake_bitset(state_, num_threads_to_wake, STATE_LOCKED | STATE_HAS_WORK);
            break;
        }
    }
}

/// Wait for work as a worker thread. If this returns true, the caller
/// holds the lock and there is work to do. If it returns false, we are
/// stopping and the caller should exit.
std::optional<ThreadPool::Task> ThreadPool::worker_wait_for_work() noexcept
{
    uint32_t old_state = state_.load(std::memory_order_relaxed);

    while (true) {
        if (old_state & STATE_STOPPING) [[unlikely]] {
            // Stopping; exit the thread immediately.
            return std::nullopt;
        }

        if ((old_state & (STATE_LOCKED | STATE_HAS_WORK)) != STATE_HAS_WORK) {
            // The lock is either held by another thread, or the queue is
            // empty. Wait until either condition changes, or we are asked
            // to stop.
            futex_wait_bitset(state_, old_state,
                              STATE_LOCKED | STATE_HAS_WORK | STATE_STOPPING);
            old_state = state_.load(std::memory_order_relaxed);
            continue;
        }

        uint32_t new_state = old_state | STATE_LOCKED;
        if (state_.compare_exchange_weak(old_state, new_state, std::memory_order_acquire,
                                         std::memory_order_relaxed)) [[likely]] {
            ThreadPool::Task task = std::move(tasks_.back());
            tasks_.pop_back();
            return task;
        }
    }
}

/// Main loop for worker threads.
void ThreadPool::worker_loop(size_t thread_id) noexcept
{
    const auto n_cpus = std::thread::hardware_concurrency();
    ASSERT(n_cpus > 0);

    // Pin each worker thread to a single CPU.
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(thread_id % n_cpus, &cpus);
    if (sched_setaffinity(0, sizeof(cpus), &cpus) < 0)
        ASSERT_MSG(false, "sched_setaffinity() failed: {}", strerror(errno));

    while (auto task = worker_wait_for_work()) {
        uint32_t mask = ~STATE_LOCKED;
        if (tasks_.empty())
            mask &= ~STATE_HAS_WORK;

        // Relinquish the lock before running the task.
        state_.fetch_and(mask, std::memory_order_release);
        futex_wake_bitset(state_, 1, STATE_LOCKED);

        task->run();
    }
}

void ThreadPool::start(size_t n_threads)
{
    ASSERT(n_threads <= UINT32_MAX);
    ASSERT_MSG(!workers_, "ThreadPool::start() called when already started!");

    n_threads_ = n_threads ? n_threads : std::thread::hardware_concurrency();
    workers_ = std::make_unique<Worker[]>(n_threads);
    for (size_t i = 0; i < n_threads; ++i)
        workers_[i].thread = std::jthread(&ThreadPool::worker_loop, this, i);
}

void ForkPoolBase::generate_victim_order(small_vector_base<uint16_t> &victim_order,
                                         size_t thread_id)
{
    ASSERT(victim_order.size() <= UINT16_MAX);
    for (size_t i = 0; i < victim_order.size(); i++)
        victim_order[i] = static_cast<uint16_t>(i);
    victim_order[thread_id] = victim_order.back();
    victim_order.pop_back();

    // Randomize the order in which each thief tries to steal work from
    // the other threads, to avoid a "convoy" of thieves hammering the
    // queues of a sequence of threads in a deterministic way.
    std::ranges::shuffle(victim_order, std::minstd_rand(thread_id));
}
