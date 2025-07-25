#ifndef DENSE_MAP_H
#define DENSE_MAP_H

#include "common.h"
#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace detail {

// NOTE: The code is dependent on the states having these specific values.
// clang-format off
enum class bucket_state : uint8_t {
    empty     = 0b00000000,
    tombstone = 0b01000000,
    sentinel  = 0b10000000,
    occupied  = 0b11000000,
};
// clang-format on

constexpr static uint8_t state_hash_mask = 0b00111111;

// How things should be aligned due to SIMD.
constexpr static size_t simd_align = 16;
constexpr static size_t max_simd_size = 32;

inline size_t find_occupied(const uint8_t *states, size_t i)
{
    for (;; i += 32) {
        const auto *p = reinterpret_cast<const __m256i *>(states + i);
        if (unsigned int mask = _mm256_movemask_epi8(_mm256_loadu_si256(p)))
            return i + std::countr_zero(mask);
    }
}

template <typename T>
struct bucket {
    alignas(T) char buffer[sizeof(T)];
    T &data() { return *reinterpret_cast<T *>(buffer); }
    const T &data() const { return *reinterpret_cast<const T *>(buffer); }
};

/// Given the size and allocation requirements described in `fields`, returns a
/// unique_ptr to a single allocation block that is large enough to contain all
/// of the described fields with the given alignments.
///
/// A pointer to each of the described fields is assigned to the corresponding
/// pointer argument given in `ptrs`; the number of pointer arguments must be
/// the same as the number of fields.
template <typename... Pointers>
__attribute__((flatten)) std::unique_ptr<std::byte[]>
compound_allocate(std::span<const std::pair<size_t, size_t>> fields, Pointers... ptrs)
{
    static_assert(sizeof...(ptrs) > 0);
    static_assert((std::is_pointer_v<Pointers> && ...));
    ASSERT(sizeof...(ptrs) == fields.size());

    // Compute the total allocation size needed.
    size_t allocation_size = fields[0].second - 1;
    for (auto [field_size, align] : fields) {
        allocation_size = (allocation_size + align - 1) & -align;
        allocation_size += field_size;
    }

    // Allocate.
    auto storage = std::make_unique_for_overwrite<std::byte[]>(allocation_size);

    // Assign the field pointers to the arguments in `ptrs'.
    uintptr_t p = reinterpret_cast<uintptr_t>(storage.get());
    size_t i = 0;
    auto assign = [&]<typename T>(T **ptr) {
        auto [size, align] = fields[i++];
        p = (p + align - 1) & -align;
        *ptr = reinterpret_cast<T *>(p);
        p += size;
    };
    (assign(ptrs), ...);

    return storage;
}

/// Array of sentinel states used for empty maps. Some operations, e.g.
/// iterators, expected the array of bucket states to be non-null, and rather
/// than introducing null checks everywhere, the states_ member points here.
constexpr auto empty_map_states = [] {
    std::array<uint8_t, max_simd_size> a;
    a.fill(static_cast<uint8_t>(bucket_state::sentinel));
    return a;
}();

} // namespace detail

template <typename Key,
          class T,
          class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>>
class dense_map {
public:
    using key_type = Key;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using difference_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = const value_type &;
    using const_reference = const value_type &;

private:
    using bucket = detail::bucket<value_type>;
    using bucket_state = detail::bucket_state;

    template <bool IsConst, typename Derived>
    class iterator_base {
        friend class dense_map;

        template <typename U>
        using Const = std::conditional_t<IsConst, const U, U>;

        iterator_base(Const<dense_map> *set, size_t index)
            : set_(set)
            , index_(index)
        {
            DEBUG_ASSERT(set);
            DEBUG_ASSERT(index <= set->capacity_);
        }

        Const<dense_map> *set_;
        size_t index_;

    public:
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key, T>;
        using pointer = Const<value_type> *;
        using reference = Const<value_type> &;

        iterator_base() = default;

        bool operator==(const iterator_base &other) const
        {
            DEBUG_ASSERT(set_ == other.set_);
            return index_ == other.index_;
        }
        bool operator!=(const iterator_base &other) const
        {
            DEBUG_ASSERT(set_ == other.set_);
            return index_ != other.index_;
        }
        reference operator*() const { return set_->buckets_[index_].data(); }
        pointer operator->() const { return &set_->buckets_[index_].data(); }
        Derived &operator++()
        {
            index_ = detail::find_occupied(set_->states_, index_ + 1);
            return static_cast<Derived &>(*this);
        }
        Derived operator++(int) { return ++Derived(*this); }
    };

public:
    class iterator : public iterator_base<false, iterator> {
        using iterator_base<false, iterator>::iterator_base;
    };
    class const_iterator : public iterator_base<true, const_iterator> {
        using iterator_base<true, const_iterator>::iterator_base;

    public:
        const_iterator(const iterator &i)
            : iterator_base<true, const_iterator>(i.set_, i.index_)
        {
        }
    };

private:
    std::unique_ptr<std::byte[]> storage_;
    bucket *buckets_;
    uint8_t *states_;
    uint32_t capacity_;
    uint32_t size_;
    uint32_t size_with_tombs_;
    [[no_unique_address]] hasher hash_;
    [[no_unique_address]] key_equal equal_;

    bucket_state state_of(size_t i) const
    {
        return static_cast<bucket_state>(states_[i] & ~detail::state_hash_mask);
    }

    void set_state_of(size_t i, bucket_state state, uint8_t hash_bits = 0)
    {
        states_[i] = static_cast<uint8_t>(state) | hash_bits;
    }

    size_t find_occupied_(size_t i) const { return detail::find_occupied(states_, i); }

    std::tuple<size_t, bool> find_bucket_with_hash_(const size_t hash,
                                                    const Key &key) const
    {
        if (capacity_ == 0) [[unlikely]] {
            // We cannot find the corresponding in an empty map, by definition.
            return {0, false};
        }

        const auto mask = capacity_ - 1;
        size_t i = hash & mask;
        uint8_t expected_state = 0b11000000 | (hash & detail::state_hash_mask);
        constexpr static size_t stride = 32;

        // Fast path before we drop into the SIMD loop: is the very first
        // bucket we landed at empty or the key we're looking for?
        if (states_[i] == 0)
            return {i, false};
        if (states_[i] == expected_state && equal_(buckets_[i].data().first, key))
            return {i, true};

        // No, unfortunately not. Skip it and start checking buckets en masse.
        i = (i + 1) & mask;

        const __m256i vexpected_state = _mm256_set1_epi8(expected_state);
        const __m256i vzero = _mm256_set1_epi8(0);

        for (;; i = (i + stride < capacity_) ? i + stride : 0) {
            // Note that if we are near the end of the table, this load will
            // also fetch a bunch of sentinel bucket states past the logical
            // end of the state array. These will never match anything, so we
            // are effectively checking fewer than 32 buckets in that case.
            const __m256i *p = reinterpret_cast<const __m256i *>(states_ + i);
            __m256i v = _mm256_loadu_si256(p);
            __m256i match = _mm256_cmpeq_epi8(v, vexpected_state);
            __m256i empty = _mm256_cmpeq_epi8(v, vzero);

            uint32_t match_mask = _mm256_movemask_epi8(match);
            uint32_t empty_mask = _mm256_movemask_epi8(empty);

            // We want to stop at the first empty bucket; mask out any matches
            // that occur after it.
            match_mask &= empty_mask ^ (empty_mask - 1);

            // At this point, `match_mask` has one bit set for each candidate
            // bucket; that is, buckets that are occupied and whose hash have
            // the same lowest 6 bits as the hash of the key we are searching
            // for. Often there will be at most one bit set in this mask, but
            // in the worst case we may have some false positives, so we need
            // to check all of them.
            for (; match_mask != 0; match_mask &= match_mask - 1) {
                int offset = std::countr_zero(match_mask);
                if (equal_(buckets_[i + offset].data().first, key))
                    return {i + offset, true};
            }

            // At this point, none of the candidate buckets we checked matched
            // the key. If there is *any* empty bucket in this group, we know
            // the key does not exist in this probe chain, since we would have
            // stopped there with a linear search.
            if (empty_mask != 0)
                return {i + std::countr_zero(empty_mask), false};
        }
    }

    std::tuple<size_t, bool, size_t> find_bucket_(const Key &key) const
    {
        const size_t hash = hash_(key);
        auto [i, found] = find_bucket_with_hash_(hash, key);
        return std::tuple(i, found, hash);
    }

    constexpr static auto max_load_ = std::make_pair(3, 4);

    void initialize_allocate(size_t new_capacity)
    {
        const std::pair<size_t, size_t> fields[] = {
            {sizeof(bucket) * new_capacity, alignof(bucket)},
            {sizeof(bucket_state) * (new_capacity + detail::max_simd_size),
             detail::simd_align},
        };
        storage_ = detail::compound_allocate(fields, &buckets_, &states_);

        capacity_ = new_capacity;
        memset(states_, 0, capacity_);
        for (size_t i = 0; i < detail::max_simd_size; i++)
            set_state_of(capacity_ + i, bucket_state::occupied);
    }

    template <typename ConstructFn>
    std::pair<iterator, bool> do_insert_helper_(const key_type &key,
                                                ConstructFn &&construct)
    {
        size_t i;
        bool found = false;
        const size_t hash = hash_(key);

        // For an empty map, the key is obviously not present, and there are
        // also no other keys that could cause a collision. In that case, we
        // can compute the resulting index and skip directly to construction of
        // the element.
        if (capacity_ == 0) [[unlikely]] {
            constexpr size_t new_capacity = 16;
            initialize_allocate(new_capacity);
            i = hash & (new_capacity - 1);
            goto insert_key;
        }

        std::tie(i, found) = find_bucket_with_hash_(hash, key);
        if (!found) {
            if (max_load_.second * (size_with_tombs_ + 1) >= capacity_ * max_load_.first)
                [[unlikely]] {
                rehash(2 * capacity_);
                i = std::get<0>(find_bucket_with_hash_(hash, key));
            }
        insert_key:
            construct(buckets_[i].buffer);
            set_state_of(i, bucket_state::occupied, hash & detail::state_hash_mask);
            size_++;
            size_with_tombs_++;
        }
        return {iterator(this, i), !found};
    }

    std::pair<iterator, bool> do_insert_(const value_type &value)
    {
        return do_insert_helper_(
            value.first, [&value](void *buffer) { new (buffer) value_type(value); });
    }

    std::pair<iterator, bool> do_insert_(value_type &&value)
    {
        return do_insert_helper_(value.first, [&value](void *buffer) {
            new (buffer) value_type(std::move(value));
        });
    }

    struct internal_tag {};

    dense_map(internal_tag,
              size_type bucket_count,
              const Hash &hash = Hash(),
              const KeyEqual &equal = KeyEqual())
        : capacity_(bucket_count)
        , size_(0)
        , size_with_tombs_(0)
        , hash_(hash)
        , equal_(equal)
    {
        initialize_allocate(bucket_count);
    }

    [[gnu::cold, gnu::noinline]] void rehash(size_type count)
    {
        dense_map new_set(internal_tag{}, count, hash_, equal_);
        for (auto &elem : *this)
            new_set.insert(std::move(elem));
        swap(new_set);
    }

public:
    //-------------------------------------------------------------------------
    // Construction and destruction.
    //-------------------------------------------------------------------------

    dense_map() noexcept
        : buckets_(nullptr)
        , states_(const_cast<uint8_t *>(detail::empty_map_states.data()))
        , capacity_(0)
        , size_(0)
        , size_with_tombs_(0)
    {
    }

    dense_map(size_type bucket_count,
              const Hash &hash = Hash(),
              const KeyEqual &equal = KeyEqual()) noexcept
        : dense_map(internal_tag{},
                    bucket_count ? std::bit_ceil(static_cast<size_type>(
                                       2 * bucket_count / max_load_factor()))
                                 : 16,
                    hash,
                    equal)
    {
        DEBUG_ASSERT(bucket_count <= UINT32_MAX);
    }

    dense_map(const dense_map &other) noexcept(
        noexcept(std::is_nothrow_copy_constructible_v<value_type>))
        : dense_map(internal_tag{}, other.capacity_, other.hash_, other.equal_)
    {
        insert(other.begin(), other.end());
    }

    dense_map &operator=(const dense_map &other) noexcept(
        noexcept(std::is_nothrow_copy_constructible_v<value_type>) &&
        noexcept(std::is_nothrow_destructible_v<value_type>))
    {
        clear();
        hash_ = other.hash_;
        equal_ = other.equal_;
        insert(other.begin(), other.end());
        return *this;
    }

    dense_map(dense_map &&other) noexcept
        : storage_(std::exchange(other.storage_, nullptr))
        , buckets_(std::exchange(other.buckets_, nullptr))
        , states_(std::exchange(other.states_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
        , size_(std::exchange(other.size_, 0))
        , size_with_tombs_(std::exchange(other.size_with_tombs_, 0))
        , hash_(std::exchange(other.hash_, Hash{}))
        , equal_(std::exchange(other.equal_, KeyEqual{}))
    {
    }

    dense_map &operator=(dense_map &&other) noexcept
    {
        swap(other);
        return *this;
    }

    template <typename InputIt>
    dense_map(InputIt begin,
              InputIt end,
              size_type bucket_count = 16,
              const Hash &hash = Hash(),
              const KeyEqual &equal =
                  KeyEqual()) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        : dense_map(bucket_count, hash, equal)
    {
        for (; begin != end; ++begin)
            insert(*begin);
    }

    dense_map(std::initializer_list<value_type> list,
              size_type bucket_count = 16,
              const Hash &hash = Hash(),
              const KeyEqual &equal =
                  KeyEqual()) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        : dense_map(list.begin(),
                    list.end(),
                    std::max<size_type>(bucket_count, list.size()),
                    hash,
                    equal)
    {
    }

    ~dense_map() = default;

    ~dense_map() noexcept(noexcept(std::is_nothrow_destructible_v<value_type>))
        requires(!std::is_trivially_destructible_v<value_type>)
    {
        // TODO: Better way to iterate all occupied buckets?
        for (size_t i = 0; i < capacity_; ++i) {
            if (state_of(i) == bucket_state::occupied)
                buckets_[i].data().~value_type();
        }
    }

    //-------------------------------------------------------------------------
    // Iterators.
    // TODO: Less stupid way to find the first element
    //-------------------------------------------------------------------------

    iterator begin() noexcept { return {this, find_occupied_(0)}; }
    const_iterator begin() const noexcept { return {this, find_occupied_(0)}; }
    const_iterator cbegin() const noexcept { return begin(); }

    iterator end() noexcept { return {this, capacity_}; }
    const_iterator end() const noexcept { return {this, capacity_}; }
    const_iterator cend() const noexcept { return {this, capacity_}; }

    //-------------------------------------------------------------------------
    // Capacity.
    //-------------------------------------------------------------------------

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }

    //-------------------------------------------------------------------------
    // Modifiers.
    //-------------------------------------------------------------------------

    void clear() noexcept
    {
        // TODO: Better way to iterate all occupied buckets?
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_t i = 0; i < capacity_; ++i) {
                if (state_of(i) == bucket_state::occupied)
                    buckets_[i].data().~value_type();
            }
        }
        memset(states_, 0, capacity_);

        size_ = 0;
        size_with_tombs_ = 0;
    }

    std::pair<iterator, bool> insert(const value_type &value)
    {
        return do_insert_(value);
    }
    std::pair<iterator, bool> insert(value_type &&value)
    {
        return do_insert_(std::move(value));
    }

    // insert() with insertion hints. The hints are just ignored, for now.
    std::pair<iterator, bool> insert(const_iterator, const value_type &value)
    {
        return do_insert_(value);
    }
    std::pair<iterator, bool> insert(const_iterator, value_type &&value)
    {
        return do_insert_(std::move(value));
    }

    template <typename InputIt>
    void insert(InputIt begin, InputIt end)
    {
        for (; begin != end; ++begin)
            do_insert_(*begin);
    }

    void insert(std::initializer_list<value_type> list)
    {
        DEBUG_ASSERT(list.size() <= UINT32_MAX);
        insert(list.begin(), list.end());
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args)
    {
        return do_insert_(value_type(std::forward<Args>(args)...));
    }

    // emplace() with insertion hints. As with insert(), the hint is ignored.
    template <typename... Args>
    std::pair<iterator, bool> emplace_hint(const_iterator, Args &&...args)
    {
        return do_insert_(value_type(std::forward<Args>(args)...));
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type &k, Args &&...args)
    {
        return do_insert_helper_(k, [&](void *buffer) {
            new (buffer) value_type(std::piecewise_construct, std::forward_as_tuple(k),
                                    std::forward_as_tuple(std::forward<Args>(args)...));
        });
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type &&k, Args &&...args)
    {
        return do_insert_helper_(k, [&](void *buffer) {
            new (buffer)
                value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)),
                           std::forward_as_tuple(std::forward<Args>(args)...));
        });
    }

    template <class... Args>
    iterator try_emplace(const_iterator, const key_type &k, Args &&...args)
    {
        return try_emplace(k, std::forward<Args>(args)...);
    }

    template <class... Args>
    iterator try_emplace(const_iterator, key_type &&k, Args &&...args)
    {
        return try_emplace(std::move(k), std::forward<Args>(args)...);
    }

    iterator erase(const_iterator pos)
    {
        DEBUG_ASSERT(pos.set_ == this);
        DEBUG_ASSERT(state_of(pos.index_) == bucket_state::occupied);

        buckets_[pos.index_].data().~value_type();
        set_state_of(pos.index_, bucket_state::tombstone);
        size_--;

        // Find the next occupied bucket.
        return {this, find_occupied_(pos.index_ + 1)};
    }

    size_type erase(const key_type &key)
    {
        if (const auto [i, found, _] = find_bucket_(key); found) {
            buckets_[i].data().~value_type();
            set_state_of(i, bucket_state::tombstone);
            size_--;
            return 1;
        }
        return 0;
    }

    void swap(dense_map &other) noexcept
    {
        using std::swap;
        swap(storage_, other.storage_);
        swap(buckets_, other.buckets_);
        swap(states_, other.states_);
        swap(capacity_, other.capacity_);
        swap(size_, other.size_);
        swap(size_with_tombs_, other.size_with_tombs_);
        swap(hash_, other.hash_);
        swap(equal_, other.equal_);
    }

    //-------------------------------------------------------------------------
    // Lookup.
    //-------------------------------------------------------------------------

    T &at(const key_type &key)
    {
        const auto [i, found, _] = find_bucket_(key);
        if constexpr (fmt::is_formattable<key_type>::value) {
            ASSERT_MSG(found, "Key '{}' not found!", key);
        } else {
            ASSERT(found);
        }
        return buckets_[i].data().second;
    }

    const T &at(const key_type &key) const
    {
        const auto [i, found, _] = find_bucket_(key);
        if constexpr (fmt::is_formattable<key_type>::value) {
            ASSERT_MSG(found, "Key '{}' not found!", key);
        } else {
            ASSERT(found);
        }
        return buckets_[i].data().second;
    }

    size_type count(const key_type &key) const
    {
        const auto [i, found, _] = find_bucket_(key);
        return found ? 1 : 0;
    }

    iterator find(const key_type &key) noexcept
    {
        const auto [i, found, _] = find_bucket_(key);
        return found ? iterator(this, i) : end();
    }

    const_iterator find(const key_type &key) const noexcept
    {
        const auto [i, found, _] = find_bucket_(key);
        return found ? const_iterator(this, i) : end();
    }

    T &operator[](const key_type &key) { return try_emplace(key).first->second; }
    T &operator[](key_type &&key) { return try_emplace(std::move(key)).first->second; }

    //-------------------------------------------------------------------------
    // Hash policy.
    //-------------------------------------------------------------------------

    float load_factor() const noexcept
    {
        return static_cast<float>(size_with_tombs_) / capacity_;
    }
    float max_load_factor() const noexcept
    {
        return static_cast<float>(max_load_.first) / max_load_.second;
    }

    void reserve(size_type count)
    {
        DEBUG_ASSERT(count <= UINT32_MAX);

        auto desired_bucket_count = std::ceil(count / max_load_factor());
        if (desired_bucket_count >= capacity_) {
            dense_map new_set(desired_bucket_count, hash_, equal_);
            for (auto &elem : *this)
                new_set.insert(std::move(elem));
            swap(new_set);
        }
    }

    //-------------------------------------------------------------------------
    // Observers.
    //-------------------------------------------------------------------------
    hasher hash_function() const { return hash_; }
    key_equal key_eq() const { return equal_; }
};

template <typename Key, class Hash, class KeyEqual>
void swap(dense_map<Key, Hash, KeyEqual> &a,
          dense_map<Key, Hash, KeyEqual> &b) noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}

#endif /* DENSE_MAP_H */
