#pragma once

#include "macros.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>

template <typename T, std::size_t Capacity>
class inplace_vector {
    static_assert(
        Capacity > 0,
        "I can't be bothered to think about the case of a vector with zero capacity.");

    alignas(T) std::byte storage_[Capacity * sizeof(T)];
    std::size_t n_ = 0;

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = T *;
    using const_iterator = const T *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    constexpr pointer ptr(size_type i) noexcept
    {
        return std::bit_cast<T *>(storage_ + sizeof(T) * i);
    }

    constexpr const_pointer ptr(size_type i) const noexcept
    {
        return std::bit_cast<T *>(storage_ + sizeof(T) * i);
    }

    template <typename... Args>
    constexpr pointer
    do_emplace_back(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        return std::construct_at(ptr(n_++), static_cast<Args &&>(args)...);
    }

    template <typename... Args>
    constexpr void
    construct_range(const size_type pos,
                    const size_type count,
                    Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        DEBUG_ASSERT(pos <= Capacity);
        DEBUG_ASSERT(pos + count <= Capacity);

        const size_type end = pos + count;
        for (size_type i = pos; i < end; ++i)
            std::construct_at(ptr(i), static_cast<Args &&>(args)...);
    }

    constexpr void destroy_range(const size_type, const size_type) noexcept
        requires(std::is_trivially_destructible_v<T>)
    {
    }

    constexpr void
    destroy_range(const size_type pos,
                  const size_type count) noexcept(std::is_nothrow_destructible_v<T>)
        requires(!std::is_trivially_destructible_v<T>)
    {
        DEBUG_ASSERT(pos <= Capacity);
        DEBUG_ASSERT(pos + count <= Capacity);

        const size_type end = pos + count;
        for (size_type i = pos; i < end; ++i)
            std::destroy_at(ptr(i));
    }

    constexpr void clear_and_set_size(const size_type new_count)
    {
        DEBUG_ASSERT(new_count <= Capacity);
        destroy_range(0, n_);
        n_ = new_count;
    }

public:
    inplace_vector() noexcept = default;

    constexpr explicit inplace_vector(const size_type count)
        : n_(count)
    {
        ASSERT(count <= Capacity);
        construct_range(0, count);
    }

    constexpr inplace_vector(const size_type count, const T &value)
        : n_(count)
    {
        ASSERT(count <= Capacity);
        construct_range(0, count, value);
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr inplace_vector(I first, S last)
    {
        for (; first != last; first++) {
            ASSERT(n_ != Capacity);
            std::construct_at(ptr(n_++), *first);
        }
    }

    template <std::ranges::input_range R>
    constexpr inplace_vector(R &&rg)
        requires(std::convertible_to<std::ranges::range_reference_t<R>, T>)
        : inplace_vector(std::ranges::begin(rg), std::ranges::end(rg))
    {
    }

    constexpr inplace_vector(std::initializer_list<T> init)
        : inplace_vector(init.begin(), init.end())
    {
    }

    // Default constructors and assignment operators used for when the more
    // constrained alternatives below are not used. This way, they can be
    // conditionally trivial depending on T.
    constexpr inplace_vector(const inplace_vector &other) = default;
    constexpr inplace_vector(inplace_vector &&other) = default;
    constexpr ~inplace_vector() = default;

    constexpr inplace_vector(const inplace_vector &other)
        requires(!std::is_trivially_copy_constructible_v<T>)
        : n_(other.n_)
    {
        DEBUG_ASSERT(other.n_ <= Capacity);
        for (size_type i = 0; i < n_; ++i)
            std::construct_at(ptr(i), other.ptr(i));
    }

    constexpr inplace_vector(inplace_vector &&other)
        requires(!std::is_trivially_move_constructible_v<T>)
        : n_(other.n_)
    {
        DEBUG_ASSERT(other.n_ <= Capacity);
        for (size_type i = 0; i < n_; ++i)
            std::construct_at(ptr(i), static_cast<T &&>(other.ptr(i)));
    }

    constexpr ~inplace_vector()
        requires(!std::is_trivially_destructible_v<T>)
    {
        destroy_range(0, n_);
    }

    constexpr inplace_vector &operator=(const inplace_vector &other)
    {
        DEBUG_ASSERT(other.n_ <= Capacity);
        clear_and_set_size(other.n_);
        std::uninitialized_copy(other.ptr(0), other.ptr(n_), ptr(0));
        return *this;
    }

    constexpr inplace_vector &operator=(inplace_vector &&other)
    {
        DEBUG_ASSERT(other.n_ <= Capacity);
        clear_and_set_size(other.n_);
        std::uninitialized_move(other.ptr(0), other.ptr(n_), ptr(0));
        return *this;
    }

    constexpr inplace_vector &operator=(std::initializer_list<T> init)
    {
        ASSERT(init.size() <= Capacity);
        clear_and_set_size(init.size());
        std::uninitialized_copy(init.begin(), init.end(), ptr(0));
        return *this;
    }

    constexpr void assign(size_type count, const T &value)
    {
        ASSERT(count <= Capacity);

        clear_and_set_size(count);
        construct_range(0, count, value);
    }

    template <std::input_iterator I>
    constexpr void assign(I first, I last)
    {
        clear();
        for (; first != last; first++) {
            ASSERT(n_ != Capacity);
            std::construct_at(ptr(n_++), *first);
        }
    }

    constexpr void assign(std::initializer_list<T> ilist)
    {
        ASSERT(ilist <= Capacity);

        clear_and_set_size(ilist.size());
        for (size_type i = 0; i < n_; ++i)
            std::construct_at(ptr(i), ilist[i]);
    }

    template <std::ranges::input_range R>
    constexpr void assign_range(R &&rg)
    {
        clear();
        assign(std::begin(rg), std::end(rg));
    }

    constexpr reference at(size_type pos)
    {
        DEBUG_ASSERT(pos < n_);
        return *ptr(pos);
    }
    constexpr const_reference at(size_type pos) const
    {
        DEBUG_ASSERT(pos < n_);
        return *ptr(pos);
    }
    constexpr reference operator[](size_type pos)
    {
        DEBUG_ASSERT(pos < n_);
        return *ptr(pos);
    }
    constexpr const_reference operator[](size_type pos) const
    {
        DEBUG_ASSERT(pos < n_);
        return *ptr(pos);
    }
    constexpr reference front()
    {
        DEBUG_ASSERT(n_);
        return *ptr(0);
    }
    constexpr const_reference front() const
    {
        DEBUG_ASSERT(n_);
        return *ptr(0);
    }
    constexpr reference back()
    {
        DEBUG_ASSERT(n_);
        return *ptr(n_ - 1);
    }
    constexpr const_reference back() const
    {
        DEBUG_ASSERT(n_);
        return *ptr(n_ - 1);
    }
    constexpr T *data() noexcept { return ptr(0); }
    constexpr const T *data() const noexcept { return ptr(0); }

    constexpr iterator begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr const_iterator cbegin() const noexcept { return data(); }

    constexpr iterator end() noexcept { return ptr(n_); }
    constexpr const_iterator end() const noexcept { return ptr(n_); }
    constexpr const_iterator cend() const noexcept { return ptr(n_); }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(begin());
    }
    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return reverse_iterator(begin());
    }

    constexpr reverse_iterator rend() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rend() const noexcept
    {
        return reverse_iterator(end());
    }
    constexpr const_reverse_iterator crend() const noexcept
    {
        return reverse_iterator(end());
    }

    constexpr bool empty() const noexcept { return n_ == 0; }
    constexpr size_type size() const noexcept { return n_; }
    static constexpr size_type max_size() noexcept { return Capacity; }
    static constexpr size_type capacity() noexcept { return Capacity; }
    static constexpr void reserve(size_type count) { DEBUG_ASSERT(count <= Capacity); }
    static constexpr void shrink_to_fit() noexcept {}

    constexpr void resize(const size_type count)
    {
        ASSERT(count <= Capacity);
        destroy_range(count, n_);
        construct_range(0, count);
        n_ = count;
    }

    constexpr void resize(const size_type count, const T &value)
    {
        ASSERT(count <= Capacity);
        destroy_range(count, n_);
        construct_range(0, count, value);
        n_ = count;
    }

    void clear() { clear_and_set_size(0); }

    template <class... Args>
    constexpr reference emplace_back(Args &&...args)
    {
        ASSERT_MSG(n_ < Capacity,
                   "emplace_back() called on a full inplace_vector ({} elements)", n_);
        return unchecked_emplace_back(static_cast<Args &&>(args)...);
    }

    template <class... Args>
    constexpr pointer try_emplace_back(Args &&...args)
    {
        return std::construct_at(ptr(n_++), static_cast<Args &&>(args)...);
    }

    template <class... Args>
    constexpr reference unchecked_emplace_back(Args &&...args)
    {
        DEBUG_ASSERT_MSG(
            n_ < Capacity,
            "unchecked_emplace_back() called on a full inplace_vector ({} elements)", n_);
        return *std::construct_at(ptr(n_++), static_cast<Args &&>(args)...);
    }

    constexpr reference push_back(const T &value)
    {
        ASSERT_MSG(n_ < Capacity,
                   "push_back() called on a full inplace_vector ({} elements)", n_);
        return unchecked_push_back(value);
    }

    constexpr reference push_back(T &&value)
    {
        ASSERT_MSG(n_ < Capacity,
                   "push_back() called on a full inplace_vector ({} elements)", n_);
        return unchecked_push_back(static_cast<T &&>(value));
    }

    constexpr pointer try_push_back(const T &value)
    {
        return n_ < Capacity ? std::construct_at(ptr(n_++), value) : nullptr;
    }

    constexpr pointer try_push_back(T &&value)
    {
        return n_ < Capacity ? std::construct_at(ptr(n_++), static_cast<T &&>(value))
                             : nullptr;
    }

    constexpr reference unchecked_push_back(const T &value)
    {
        DEBUG_ASSERT_MSG(
            n_ < Capacity,
            "unchecked_push_back() called on a full inplace_vector ({} elements)", n_);
        return *std::construct_at(ptr(n_++), value);
    }

    constexpr reference unchecked_push_back(T &&value)
    {
        DEBUG_ASSERT_MSG(
            n_ < Capacity,
            "unchecked_push_back() called on a full inplace_vector ({} elements)", n_);
        return *std::construct_at(ptr(n_++), static_cast<T &&>(value));
    }

    constexpr void pop_back() noexcept
    {
        DEBUG_ASSERT(!empty());
        std::destroy_at(ptr(--n_));
    }

    [[gnu::always_inline]] constexpr iterator erase(const_iterator pos_)
    {
        return erase(pos_, pos_ + 1);
    }

    constexpr iterator erase(const_iterator first_, const_iterator last_)
    {
        DEBUG_ASSERT(first_ <= last_);
        DEBUG_ASSERT(first_ >= ptr(0));
        DEBUG_ASSERT(last_ <= ptr(n_));

        iterator p = const_cast<iterator>(first_);
        iterator q = const_cast<iterator>(last_);
        const size_type count = q - p;
        const size_type tail_count = end() - q;

        for (size_type i = 0; i < tail_count; ++i)
            p[i] = static_cast<T &&>(q[i]);

        n_ -= count;
        return p;
    }

    constexpr void swap(inplace_vector &other) noexcept(noexcept(
        Capacity == 0 ||
        (std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)))
    {
        using std::swap;

        inplace_vector &a = n_ < other.n_ ? *this : other;
        inplace_vector &b = n_ < other.n_ ? other : *this;

        for (size_type i = a.n_; i < b.n_; ++i)
            std::construct_at(a.ptr(i), static_cast<T &&>(*b.ptr(i)));
        b.destroy_range(a.n_, b.n_);

        for (size_t i = 0; i < a.n_; ++i)
            swap(*a.ptr(i), *b.ptr(i));
        swap(a.n_, b.n_);
    }

    constexpr friend bool operator==(const inplace_vector<T, Capacity> &lhs,
                                     const inplace_vector<T, Capacity> &rhs)
        requires(std::equality_comparable<T>)
    {
        return lhs.size() == rhs.size() &&
               std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
};

// Ensure that the copy/move constructors of inplace_vector is trivial if and
// only if T is also trivially so.
static_assert(std::is_trivially_copy_constructible_v<inplace_vector<int, 2>>);
static_assert(std::is_trivially_move_constructible_v<inplace_vector<int, 2>>);

template <typename T, size_t N>
void std::swap(inplace_vector<T, N> &a, inplace_vector<T, N> &b)
{
    a.swap(b);
}

template <class T, std::size_t N, class U = T>
constexpr typename inplace_vector<T, N>::size_type erase(inplace_vector<T, N> &c,
                                                         const U &value)
{
    auto it = std::remove(c.begin(), c.end(), value);
    auto r = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}

template <class T, std::size_t N, class Pred>
constexpr typename inplace_vector<T, N>::size_type erase_if(inplace_vector<T, N> &c,
                                                            Pred pred)
{
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto r = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}
