#pragma once

#include "macros.h"
#include "small_vector.h"
#include <atomic>
#include <cerrno>
#include <linux/futex.h>
#include <memory>
#include <mutex>
#include <sched.h>
#include <span>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

/// Needlessly complex and probably horribly broken thread pool implementation
/// that relies on manual futex management and manual type erasure instead of
/// using std::function. (It was fun to write, at least.)
class ThreadPool {
private:
    static void futex_wake(const std::atomic_uint32_t &addr, int32_t n) noexcept
    {
        if (syscall(SYS_futex, &addr, FUTEX_WAKE_PRIVATE, n) < 0)
            ASSERT_MSG(false, "futex(FUTEX_WAKE_PRIVATE) failed: {}", strerror(errno));
    }

    static void futex_wake_bitset(const std::atomic_uint32_t &addr,
                                  int32_t n,
                                  uint32_t bitset) noexcept
    {
        if (syscall(SYS_futex, &addr, FUTEX_WAKE_BITSET_PRIVATE, n, nullptr, nullptr,
                    bitset) < 0)
            ASSERT_MSG(false, "futex(FUTEX_WAKE_BITSET_PRIVATE) failed: {}",
                       strerror(errno));
    }

    static bool futex_wait(const std::atomic_uint32_t &addr, uint32_t expected) noexcept
    {
        if (syscall(SYS_futex, &addr, FUTEX_WAIT_PRIVATE, expected, nullptr) < 0) {
            ASSERT_MSG(errno == EAGAIN || errno == EINTR,
                       "futex(FUTEX_WAIT_PRIVATE) failed: {}", strerror(errno));
            return false;
        }

        return true;
    }

    static bool futex_wait_bitset(const std::atomic_uint32_t &addr,
                                  uint32_t expected,
                                  uint32_t bitset) noexcept
    {
        if (syscall(SYS_futex, &addr, FUTEX_WAIT_BITSET_PRIVATE, expected, nullptr,
                    nullptr, bitset) < 0) {
            ASSERT_MSG(errno == EAGAIN || errno == EINTR,
                       "futex(FUTEX_WAIT_BITSET_PRIVATE) failed: {}", strerror(errno));
            return false;
        }

        return true;
    }

    /// Wait until the given atomic counter becomes zero.
    static void atomic_wait_zero(const std::atomic_uint32_t &counter) noexcept
    {
        while (true) {
            uint32_t val = counter.load(std::memory_order_acquire);
            if (val == 0)
                break;
            futex_wait(counter, val);
        }
    }

    /// Represents a single unit of work to be executed by a worker thread.
    struct alignas(64) Task {
        enum class Type {
            for_each_index,
            for_each_thread,
        };

        Task::Type type;

        /// Type-erased function pointer to execute the task.
        union {
            void (*for_each_index)(void *, size_t, size_t);
            void (*for_each_thread)(void *, size_t);
        } work_fn;

        /// Type-erased data to pass to the appropriate function in `work_fn`.
        std::unique_ptr<void, void (*)(void *)> data;

        size_t begin;                     // for_each_index
        size_t end;                       // for_each_index
        size_t thread_id;                 // for_each_thread
        std::atomic_uint32_t *futex_word; // for_each_index, for_each_thread

        void run()
        {
            ASSERT(data);

            switch (type) {
            case Task::Type::for_each_thread:
                ASSERT(work_fn.for_each_thread);
                ASSERT(futex_word);
                work_fn.for_each_thread(data.get(), thread_id);
                if (futex_word->fetch_sub(1, std::memory_order_release) == 1)
                    futex_wake(*futex_word, INT_MAX);
                break;

            case Task::Type::for_each_index:
                ASSERT(work_fn.for_each_index);
                ASSERT(futex_word);
                work_fn.for_each_index(data.get(), begin, end);
                if (futex_word->fetch_sub(1, std::memory_order_release) == 1)
                    futex_wake(*futex_word, INT_MAX);
                break;
            }
        }
    };

    // Bits in state_:
    enum {
        // This bit is set when the lock is held.
        STATE_LOCKED = 1U << 0,

        // This bit is set when the thread pool is stopping.
        STATE_STOPPING = 1U << 1,

        // This bit is set when the work queue is non-empty. Worker threads
        // wait on this bit to be signaled when new work is added; the main
        // thread wakes worker threads when new work is added.
        STATE_HAS_WORK = 1U << 2,
    };

    // Read-write by all threads:
    small_vector<Task, 32> tasks_;
    std::atomic_uint32_t state_ = 0;

    // Mostly read-only:
    alignas(64);
    std::unique_ptr<std::thread[]> threads_;
    size_t n_threads_;

    /// Acquire the thread pool lock.
    void lock() noexcept
    {
        uint32_t old_state = state_.fetch_or(STATE_LOCKED, std::memory_order_acquire);

        while (old_state & STATE_LOCKED) [[unlikely]] {
            futex_wait_bitset(state_, old_state | STATE_LOCKED, STATE_LOCKED);
            old_state = state_.fetch_or(STATE_LOCKED, std::memory_order_acquire);
        }
    }

    /// Unlock the thread pool and mark the work queue as being non-empty.
    void unlock_with_work(size_t num_threads_to_wake = INT_MAX) noexcept
    {
        uint32_t old_state = state_.load(std::memory_order_relaxed);

        while (true) {
            auto new_state = old_state;
            new_state &= ~STATE_LOCKED;
            new_state |= STATE_HAS_WORK;
            if (state_.compare_exchange_weak(old_state, new_state,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)) [[likely]] {
                futex_wake_bitset(state_, num_threads_to_wake,
                                  STATE_LOCKED | STATE_HAS_WORK);
                break;
            }
        }
    }

    /// Wait for work as a worker thread. If this returns true, the caller
    /// holds the lock and there is work to do. If it returns false, we are
    /// stopping and the caller should exit.
    std::optional<Task> worker_wait_for_work() noexcept
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
            if (state_.compare_exchange_weak(old_state, new_state,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed)) [[likely]] {
                Task task = std::move(tasks_.back());
                tasks_.pop_back();
                return task;
            }
        }
    }

    /// Main loop for worker threads.
    void worker_loop(size_t thread_id) noexcept
    {
        // Pin each worker thread to a single CPU.
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(thread_id % std::thread::hardware_concurrency(), &cpus);
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

    ThreadPool() = default;

    ~ThreadPool()
    {
        state_.fetch_or(STATE_STOPPING, std::memory_order_seq_cst);
        futex_wake_bitset(state_, INT_MAX, STATE_STOPPING);

        for (size_t i = 0; i < n_threads_; ++i)
            threads_[i].join();
    }

public:
    static ThreadPool &get()
    {
        static ThreadPool instance;
        return instance;
    }

    size_t num_threads() const noexcept { return n_threads_; }

    void start(size_t n_threads = std::thread::hardware_concurrency())
    {
        ASSERT_MSG(!threads_, "ThreadPool::start() called when already started!");

        n_threads_ = n_threads;
        threads_ = std::make_unique<std::thread[]>(n_threads);
        for (size_t i = 0; i < n_threads; ++i)
            threads_[i] = std::thread(&ThreadPool::worker_loop, this, i);
    }

    template <typename Container,
              std::invocable<
                  std::span<typename std::remove_reference_t<Container>::value_type>> Fn>
    void for_each(Container &&container, Fn &&fn)
        requires(std::convertible_to<
                 Container,
                 std::span<typename std::remove_reference_t<Container>::value_type>>)
    {
        ASSERT_MSG(threads_, "ThreadPool::for_each() called when not started!");
        for_each_index(0, std::size(container),
                       [&container, f = std::forward<Fn>(fn)](size_t begin, size_t end) {
                           std::span span(container);
                           f(span.subspan(begin, end - begin));
                       });
    }

    template <std::invocable<size_t, size_t> Fn>
    void for_each_index(size_t begin, size_t end, Fn &&fn)
    {
        ASSERT_MSG(threads_, "ThreadPool::for_each_index() called when not started!");
        ASSERT(begin <= end);

        std::atomic_uint32_t remaining = n_threads_;

        auto work_fn = [](void *p, size_t begin, size_t end) {
            (*static_cast<Fn *>(p))(begin, end);
        };

        lock();
        tasks_.reserve(tasks_.size() + n_threads_);

        const size_t slice_size = ((end - begin) / n_threads_) + 1;
        for (size_t i = 0; i < n_threads_; ++i) {
            const size_t local_begin = i * slice_size;
            const size_t local_end = std::min((i + 1) * slice_size, end);
            tasks_.push_back(Task{
                .type = Task::Type::for_each_index,
                .work_fn = {.for_each_index = work_fn},
                .data = std::unique_ptr<void, void (*)(void *)>(
                    new Fn(fn), [](void *p) { delete static_cast<Fn *>(p); }),
                .begin = local_begin,
                .end = local_end,
                .futex_word = &remaining,
            });
        }

        unlock_with_work(n_threads_);
        atomic_wait_zero(remaining);
    }

    /// Invoke the given function once on each worker thread. The function
    /// receives the thread ID, an integer in the range [0, num_threads()), as
    /// its only argument. Blocks until all threads have completed.
    template <std::invocable<size_t> Fn>
    void for_each_thread(Fn &&fn)
    {
        ASSERT_MSG(threads_, "ThreadPool::for_each_index() called when not started!");

        std::atomic_uint32_t remaining = n_threads_;

        auto work_fn = [](void *p, size_t thread_id) {
            (*static_cast<Fn *>(p))(thread_id);
        };

        lock();
        tasks_.reserve(tasks_.size() + n_threads_);

        for (size_t i = 0; i < n_threads_; ++i) {
            tasks_.push_back(Task{
                .type = Task::Type::for_each_thread,
                .work_fn = {.for_each_thread = work_fn},
                .data = std::unique_ptr<void, void (*)(void *)>(
                    new Fn(fn), [](void *p) { delete static_cast<Fn *>(p); }),
                .thread_id = i,
                .futex_word = &remaining,
            });
        }

        unlock_with_work(n_threads_);
        atomic_wait_zero(remaining);
    }
};

template <typename T>
void atomic_store_max(std::atomic<T> &a,
                      const T b,
                      std::memory_order order = std::memory_order_seq_cst) noexcept
{
    auto value = a.load(order);
    while (b > value &&
           !a.compare_exchange_weak(value, b, order, std::memory_order_relaxed))
        ;
}
