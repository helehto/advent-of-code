#pragma once

#include "macros.h"
#include "small_vector.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <linux/futex.h>
#include <memory>
#include <optional>
#include <ranges>
#include <sched.h>
#include <span>
#include <sys/syscall.h>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <vector>

#ifdef __AVX__
#include <immintrin.h>
#endif

// The futex wrappers take a reference to a std::atomic_uint32_t; the Linux
// syscalls on the other hand take a pointer to a 32-bit word. Assert that this
// works out in practice.
static_assert(sizeof(std::atomic_uint32_t) == sizeof(uint32_t));
static_assert(alignof(std::atomic_uint32_t) >= alignof(uint32_t));

void futex_wake(const std::atomic_uint32_t &addr, int32_t n) noexcept;
void futex_wake_bitset(const std::atomic_uint32_t &addr,
                       int32_t n,
                       uint32_t bitset) noexcept;
bool futex_wait(const std::atomic_uint32_t &addr, uint32_t expected) noexcept;
bool futex_wait_bitset(const std::atomic_uint32_t &addr,
                       uint32_t expected,
                       uint32_t bitset) noexcept;

/// Wait until the given atomic counter becomes zero, waiting on its
/// address as a futex.
void atomic_wait_zero(const std::atomic_uint32_t &counter,
                      std::memory_order order = std::memory_order_seq_cst) noexcept;

/// Whether the current thread is a worker thread and currently executing a
/// task.
inline thread_local bool g_executing_thread_pool_task = false;

/// Needlessly complex and probably horribly broken thread pool implementation
/// that relies on manual futex management and manual type erasure instead of
/// using std::function. (It was fun to write, at least.)
class ThreadPool {
private:
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
                g_executing_thread_pool_task = true;
                work_fn.for_each_thread(data.get(), thread_id);
                g_executing_thread_pool_task = false;
                if (futex_word->fetch_sub(1, std::memory_order_release) == 1)
                    futex_wake(*futex_word, INT_MAX);
                break;

            case Task::Type::for_each_index:
                ASSERT(work_fn.for_each_index);
                ASSERT(futex_word);
                g_executing_thread_pool_task = true;
                work_fn.for_each_index(data.get(), begin, end);
                g_executing_thread_pool_task = false;
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
    alignas(64) std::unique_ptr<std::thread[]> threads_;
    size_t n_threads_ = 0;

    void lock() noexcept;
    void unlock_with_work(size_t num_threads_to_wake = INT_MAX) noexcept;
    std::optional<Task> worker_wait_for_work() noexcept;
    void worker_loop(size_t thread_id) noexcept;

    ThreadPool();
    ~ThreadPool();

public:
    static ThreadPool &get()
    {
        static ThreadPool instance;
        return instance;
    }

    size_t num_threads() const noexcept { return n_threads_; }

    /// Start the thread pool with the given number of threads. If n_threads is
    /// 0, use the number of processors available to the process.
    void start(size_t n_threads = 0);

    template <std::ranges::contiguous_range Range, typename Fn>
    void for_each(Range &&r, Fn &&fn)
        requires(
            std::invocable<Fn, const std::decay_t<std::ranges::range_value_t<Range>> &> &&
            std::copy_constructible<Fn>)
    {
        ASSERT_MSG(threads_, "ThreadPool::for_each() called when not started!");
        for_each_index(0zu, std::ranges::size(r),
                       [&r, f = std::forward<Fn>(fn)](size_t begin, size_t end) {
                           const auto *data = std::ranges::data(r);
                           for (size_t i = begin; i < end; ++i)
                               f(data[i]);
                       });
    }

    template <std::ranges::contiguous_range Range, typename Fn>
    void for_each_slice(Range &&r, Fn &&fn)
        requires(std::invocable<
                     Fn,
                     std::span<const std::decay_t<std::ranges::range_value_t<Range>>>> &&
                 std::copy_constructible<Fn>)
    {
        ASSERT_MSG(threads_, "ThreadPool::for_each_slice() called when not started!");
        for_each_index(0zu, std::ranges::size(r),
                       [&r, f = std::forward<Fn>(fn)](size_t begin, size_t end) {
                           const auto *data = std::ranges::data(r) + begin;
                           const size_t size = end - begin;
                           f(std::span(data, size));
                       });
    }

    template <typename FnRef, typename Fn = std::decay_t<FnRef>>
    void for_each_index(size_t begin, size_t end, FnRef &&fn)
        requires(std::invocable<Fn, size_t, size_t> && std::copy_constructible<Fn>)
    {
        ASSERT_MSG(
            !g_executing_thread_pool_task,
            "ThreadPool::for_each_index() must not be called from a worker thread!");
        ASSERT_MSG(threads_, "ThreadPool::for_each_index() called when not started!");
        ASSERT(begin <= end);

        std::atomic_uint32_t remaining = n_threads_;

        auto work_fn = [](void *p, size_t begin, size_t end) {
            (*static_cast<Fn *>(p))(begin, end);
        };

        lock();
        tasks_.reserve(tasks_.size() + n_threads_);

        for (size_t i = 0; i < n_threads_; ++i) {
            const size_t local_begin = i * (end - begin) / n_threads_ + begin;
            const size_t local_end = (i + 1) * (end - begin) / n_threads_ + begin;
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
    template <typename FnRef, typename Fn = std::decay_t<FnRef>>
    void for_each_thread(FnRef &&fn)
        requires(std::invocable<Fn, size_t> && std::copy_constructible<Fn>)
    {
        ASSERT_MSG(
            !g_executing_thread_pool_task,
            "ThreadPool::for_each_thread() must not be called from a worker thread!");
        ASSERT_MSG(threads_, "ThreadPool::for_each_thread() called when not started!");

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

/// Store `min(a, b)` into `a` atomically, returning the previous value of `a`.
/// Memory is affected according to `order`.
template <typename T>
T atomic_fetch_min(std::atomic<T> *obj,
                   typename std::atomic<T>::value_type arg,
                   std::memory_order order = std::memory_order_seq_cst)
{
    DEBUG_ASSERT(obj);
    auto value = obj->load(std::memory_order_relaxed);
    while (true) {
        if (arg >= value)
            return value;
        if (obj->compare_exchange_weak(value, arg, order, std::memory_order_relaxed))
            return value;
    }
}

/// Store `max(a, b)` into `a` atomically, returning the previous value of `a`.
/// Memory is affected according to `order`.
template <typename T>
T atomic_fetch_max(std::atomic<T> *obj,
                   typename std::atomic<T>::value_type arg,
                   std::memory_order order = std::memory_order_seq_cst)
{
    DEBUG_ASSERT(obj);
    auto value = obj->load(std::memory_order_relaxed);
    while (true) {
        if (arg <= value)
            return value;
        if (obj->compare_exchange_weak(value, arg, order, std::memory_order_relaxed))
            return value;
    }
}

/// Implementation of a concurrent lock-free deque.
///
/// See "Dynamic Circular Work-Stealing Deque" by Chase and Lev (2005) for the
/// initial paper describing this algorithm. This particular implementation is
/// based on the the pseudocode in "Correct and Efficient Work-Stealing for
/// Weak Memory Models" by Lê et al (2013).
template <typename T>
struct ChaseLevDeque {
private:
    static_assert(std::is_nothrow_default_constructible_v<T>);
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(!std::is_const_v<T>);
    static_assert(!std::is_volatile_v<T>);
    static_assert(sizeof(T) <= 16);

    /// 64-bit indices makes it practically impossible for unmasked indices to
    /// wrap.
    using size_type = uint64_t;

    struct WorkQueueArray {
        /// Wrapper for T to ensure correct alignment for std::atomic_ref<T>.
        struct Item {
            alignas(std::atomic_ref<T>::required_alignment) T value;
        };

        std::atomic<size_type> mask; // size-1 to replace n%size with n&mask
        WorkQueueArray *next;        // linked list of unused arrays
        mutable Item items[];

        T load(size_t index) const noexcept
        {
#ifdef __AVX__
            if constexpr (sizeof(T) == 16) {
                // On platforms with AVX, aligned 16-byte loads/stores are
                // atomic on all relevant platforms.
                // (cf. <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104688>)
                //
                // On GCC, using std::atomic_ref here generates a call to
                // __atomic_load_16, ending up in libatomic. It does runtime
                // detection to make a load just a vmovdqa if AVX is available,
                // but the call via the PLT adds significant overhead. We
                // assume that AVX is available, so load using inline assembly
                // to avoid that.
                __m128i x;
                asm("vmovdqa %1, %0" : "=x"(x) : "m"(items[index].value));
                T value;
                _mm_storeu_si128(reinterpret_cast<__m128i *>(&value), x);
                return value;
            }
#endif

            return std::atomic_ref<T>(items[index].value).load(std::memory_order_relaxed);
        }

        T load_unmasked(size_type index) const noexcept
        {
            return load(index & mask.load(std::memory_order_relaxed));
        }

        void store(size_t index, const T &value) noexcept
        {
#ifdef __AVX__
            if constexpr (sizeof(T) == 16) {
                // See the comment in load(). The same reasoning applies here;
                // additionally, libatomic executes an mfence after the store
                // (vmovdqa) which is entirely unnecessary here -- all stores
                // to the items array are ordered properly by surrounding
                // stores/fences.
                __m128i x = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&value));
                asm("vmovdqa %1, %0" : "=m"(items[index].value) : "x"(x));
                return;
            }
#endif

            std::atomic_ref<T>(items[index].value)
                .store(value, std::memory_order_relaxed);
        }
    };

    static constexpr size_t initial_buffer_size = 64;

    /// A pointer to the backing ring buffer holding the current items in the
    /// deque.
    ///
    /// This pointer is read by both owner and thief threads, but only written
    /// by the owner thread when the buffer needs to grow.
    alignas(64) std::atomic<WorkQueueArray *> array = nullptr;

    /// The unmasked `bottom` index refers to one past the last valid item in
    /// the ring buffer, i.e. the slot which will be written by push(), or one
    /// past the slot which will be read by pop() if the queue is non-empty.
    ///
    /// This is only read or written by the owner thread.
    alignas(64) std::atomic<size_type> bottom = 0;

    /// The unmasked `top` index refers to the first valid item in the circular
    /// array, i.e. the slot which will be read by steal().
    ///
    /// This is read and written by both owner and thief threads.
    alignas(64) std::atomic<size_type> top = 0;

    static WorkQueueArray *allocate_array(size_t size)
    {
        auto *const a = reinterpret_cast<WorkQueueArray *>(operator new(
            sizeof(WorkQueueArray) + sizeof(typename WorkQueueArray::Item) * size,
            std::align_val_t(alignof(WorkQueueArray))));
        a->mask.store(size - 1, std::memory_order_relaxed);
        a->next = nullptr;
        return a;
    }

    /// Grow the backing ring buffer. Must only be called by the owner thread.
    [[gnu::noinline]]
    WorkQueueArray *grow(size_type b, size_type t)
    {
        auto *const old_array = array.load(std::memory_order_acquire);
        const auto old_mask = old_array->mask.load(std::memory_order_relaxed);
        const auto old_size = old_mask + 1;
        const auto new_size = old_size * 2;
        const auto new_mask = new_size - 1;
        auto *const new_array = allocate_array(new_size);

        for (size_t i = t; i < b; ++i) {
            const T item = old_array->load(i & old_mask);
            new_array->store(i & new_mask, item);
        }

        // Publish the new array. We can't free the old array immediately since
        // other threads may still be reading from it, so just hook it into a
        // linked list for later reclamation.
        new_array->next = old_array;
        array.store(new_array, std::memory_order_release);

        return new_array;
    }

public:
    ChaseLevDeque()
        : array(allocate_array(initial_buffer_size))
    {
    }

    ChaseLevDeque(const ChaseLevDeque &) = delete;
    ChaseLevDeque &operator=(const ChaseLevDeque &) = delete;
    ChaseLevDeque(ChaseLevDeque &&) = delete;
    ChaseLevDeque &operator=(ChaseLevDeque &&) = delete;

    // The destructor does nothing; it is up to the individual worker threads
    // to call destroy() before exiting to free all allocated memory.
    ~ChaseLevDeque() = default;

    /// Pop an item from the deque. Must only be called by the owner thread.
    bool pop(T &result) noexcept
    {
        const auto b = bottom.load(std::memory_order_relaxed) - 1;
        auto *const a = array.load(std::memory_order_relaxed);
        bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        auto t = top.load(std::memory_order_relaxed);
        const std::make_signed_t<size_type> new_size = b - t;

        // If the queue is empty, first set `bottom <- top` to reset the queue
        // to the canonical empty state.
        if (new_size < 0) {
            bottom.store(t, std::memory_order_relaxed);
            return false;
        }

        // The queue was not empty at time of reading `top`. After reading the
        // item, there are two cases:
        //
        //   (1) There are more items left in the queue. In this case, the item
        //   we just took is valid, so we are done.
        //
        //   (2) This was the last item in the queue. In this case, there is a
        //   risk that a conurrent steal() operation won the race to take the
        //   same item.
        //
        result = a->load_unmasked(b);
        if (new_size > 0)
            return true;

        // Last item in queue; try to claim it by incrementing `top`. If a
        // thief won the race, this CAS will always fail (`top` is only ever
        // incremented, so the ABA problem becomes impossible by design).
        const bool got_item = top.compare_exchange_strong(
            t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);

        // Regardless of whether we got the last item (CAS succeeded) or a
        // thief did (CAS failed), the queue is now empty.
        bottom.store(b + 1, std::memory_order_relaxed);
        return got_item;
    }

    /// Push an item onto the deque. Must only be called by the owner thread.
    void push(const T &item)
    {
        const auto b = bottom.load(std::memory_order_relaxed);
        const auto t = top.load(std::memory_order_acquire);
        auto *a = array.load(std::memory_order_relaxed);
        auto mask = a->mask.load(std::memory_order_relaxed);
        const auto size = mask + 1;

        // Check against size-1 to leave at least one slot open.
        if (b - t > size - 1) [[unlikely]] {
            a = grow(b, t);
            mask = a->mask.load(std::memory_order_relaxed);
        }

        a->store(b & mask, item);
        std::atomic_thread_fence(std::memory_order_release);
        bottom.store(b + 1, std::memory_order_relaxed);
    }

    /// Steal an item from the deque.
    bool steal(T &result) noexcept
    {
        auto t = top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const auto b = bottom.load(std::memory_order_acquire);

        const std::make_signed_t<size_type> size = b - t;
        if (size <= 0)
            return false;

        auto *const a = array.load(std::memory_order_acquire);
        result = a->load_unmasked(t);

        // Try to claim the item by incrementing `top`. This can fail if the
        // owner or another thief beat us to it after we read `top`, which the
        // caller will have to handle.
        return top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst,
                                           std::memory_order_relaxed);
    }

    /// Check if the deque is empty. May be called by any thread.
    bool empty() const noexcept
    {
        auto t = top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const auto b = bottom.load(std::memory_order_acquire);
        const std::make_signed_t<size_type> size = b - t;
        return size <= 0;
    }

    /// Free all allocated memory. May only be called from the owner thread;
    /// after this, the deque must not be used again.
    void destroy() noexcept
    {
        // Free the linked list of arrays.
        auto *a = array.exchange(nullptr, std::memory_order_relaxed);
        while (a != nullptr) {
            auto *const next = a->next;
            operator delete(a, std::align_val_t(alignof(WorkQueueArray)));
            a = next;
        }
    }
};

struct ForkPoolBase {
protected:
    static void generate_victim_order(small_vector_base<uint16_t> &victim_order,
                                      size_t thread_id);
};

template <typename State>
struct ForkPool : private ForkPoolBase {
    alignas(64) ThreadPool *pool;
    size_t n_threads;
    std::vector<ChaseLevDeque<State>> work_queues;

    alignas(64) std::atomic<size_t> global_idle_count = 0;
    alignas(64) std::atomic_flag do_terminate = false;
    alignas(64) std::atomic<size_t> g_terminator_thread_id = SIZE_MAX;

    bool all_work_queues_empty() const noexcept
    {
        for (size_t i = 0; i < n_threads; i++)
            if (!work_queues[i].empty())
                return false;

        std::atomic_thread_fence(std::memory_order_seq_cst);

        // Check again. (Why!?)
        for (size_t i = 0; i < n_threads; i++)
            if (!work_queues[i].empty())
                return false;

        return true;
    }

    bool try_terminate(size_t thread_id) noexcept
    {
        // If there are still active threads, we cannot terminate yet.
        if (global_idle_count.load(std::memory_order_acquire) < n_threads)
            return false;

        // Try to become the terminator thread. This CAS ensures that only one
        // thread becomes the terminator.
        size_t old = SIZE_MAX;
        if (!g_terminator_thread_id.compare_exchange_strong(
                old, thread_id, std::memory_order_acquire, std::memory_order_relaxed))
            return false;

        if (all_work_queues_empty()) {
            do_terminate.test_and_set(std::memory_order_release);
            return true;
        }

        // Relinquish terminator role.
        g_terminator_thread_id.store(SIZE_MAX, std::memory_order_release);
        return false;
    }

public:
    struct TaskContext {
        size_t thread_id;
        small_vector_base<State> &next;
    };

    ForkPool(ThreadPool &pool)
        : pool(&pool)
        , n_threads(pool.num_threads())
        , work_queues(pool.num_threads())
    {
    }

    ForkPool(const ForkPool &) = delete;
    ForkPool &operator=(const ForkPool &) = delete;
    ForkPool(ForkPool &&) = delete;
    ForkPool &operator=(ForkPool &&) = delete;

    void push(std::initializer_list<State> initial_items) noexcept
    {
        push(std::span(initial_items));
    }

    void push(std::span<const State> initial_items) noexcept
    {
        for (size_t i = 0; i < initial_items.size(); i++) {
            // Round-robin to spread out the initial work.
            work_queues[i % n_threads].push(initial_items[i]);
        }
    }

    void run(auto &&work_fn)
    {
        ASSERT_MSG(!do_terminate.test(), "ForkPool is one-shot and cannot be reused.");
        std::atomic_uint32_t remaining_threads = n_threads;

        pool->for_each_thread([&, fn = work_fn](size_t thread_id) noexcept {
            bool is_idle = false;
            auto &queue = work_queues[thread_id];

            small_vector<uint16_t, 64> victim_order(n_threads);
            generate_victim_order(victim_order, thread_id);

            small_vector<State, 32> spawned_tasks;
            while (!do_terminate.test(std::memory_order_acquire)) {
                State u;
                while (queue.pop(u)) {
                restart_with_new_work:
                    spawned_tasks.clear();
                    TaskContext ctx{thread_id, spawned_tasks};
                    fn(ctx, std::move(u));

                    if (!spawned_tasks.empty()) {
                        u = spawned_tasks[0];
                        for (size_t i = 1; i < spawned_tasks.size(); i++)
                            queue.push(spawned_tasks[i]);

                        if (is_idle) {
                            is_idle = false;
                            global_idle_count.fetch_sub(1, std::memory_order_release);
                        }

                        goto restart_with_new_work;
                    }
                }

                // We are out of local work. Try to steal work from other threads.
                for (size_t i = 0; i < 32; i++) {
                    for (const size_t victim : victim_order) {
                        if (work_queues[victim].steal(u))
                            goto restart_with_new_work;
                    }
                    std::this_thread::yield();
                }

                // We failed to steal work. Become idle and check if we can terminate.
                if (!is_idle) {
                    is_idle = true;
                    global_idle_count.fetch_add(1, std::memory_order_relaxed);
                }
                if (try_terminate(thread_id))
                    break;

                std::this_thread::yield();
            }

            if (remaining_threads.fetch_sub(1, std::memory_order_release) == 1)
                futex_wake(remaining_threads, INT_MAX);
            else
                atomic_wait_zero(remaining_threads);

            queue.destroy();
        });
    }
};
