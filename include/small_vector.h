#pragma once

#include "macros.h"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <memory>

namespace detail {

/// Base class for all aspects of small_vector which do not depend on any
/// template parameter.
class untyped_small_vector {
protected:
    // This will either point to the inline buffer in small_vector<T,N>, or a
    // separately allocated heap buffer.
    void *data_;
    uint32_t n_;
    uint32_t capacity_;

    constexpr untyped_small_vector(void *data, uint32_t n, uint32_t capacity)
        : data_(data)
        , n_(n)
        , capacity_(capacity)
    {
    }

public:
    constexpr bool empty() const noexcept { return n_ == 0; }
    constexpr size_t size() const noexcept { return n_; }
    static constexpr size_t max_size() noexcept { return UINT32_MAX; }
    constexpr size_t capacity() const noexcept { return capacity_; }
};

/// The default number of inline elements for a small_vector<T>.
template <typename T>
consteval size_t default_inline_capacity()
{
    // one cache line or so
    constexpr size_t n = 64 / sizeof(T);
    return n > 0 ? n : 1;
}

} // namespace detail

/// Base class for all aspects of small_vector which only depend on the
/// particular type being stored, but not the capacity of the inline buffer.
/// References to this type can be passed to functions to operate on a
/// small_vector regardless of its specific inline capacity.
template <typename T>
class small_vector_base : public detail::untyped_small_vector {
    // Note: we assume that the inline buffer in small_vector is located after
    // small_vector_base in memory (see get_inline_buffer()). This means that
    // small_vector_base MUST NOT contain any member variables.

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

protected:
    constexpr small_vector_base(T *data, uint32_t n, uint32_t capacity)
        : untyped_small_vector(static_cast<void *>(data), n, capacity)
    {
    }

    // Copying and moving is handled by small_vector<T,N>.
    small_vector_base(const small_vector_base &) = delete;
    small_vector_base &operator=(const small_vector_base &) = delete;
    small_vector_base(small_vector_base &&) = delete;
    small_vector_base &operator=(small_vector_base &&) = delete;

    constexpr ~small_vector_base()
    {
        std::destroy_n(data(), n_);
        delete_buffer(data_);
    }

    static constexpr T *allocate_buffer(const size_t n)
    {
        void *p = operator new[](n * sizeof(T), std::align_val_t(alignof(T)));
        return static_cast<T *>(p);
    }

    constexpr void delete_buffer(void *p)
    {
        if (p != inline_buffer())
            operator delete[](p, std::align_val_t(alignof(T)));
    }

    constexpr T *inline_buffer()
    {
        struct X {
            union {
                small_vector_base x;
            };
            alignas(T) std::byte buf[1];
        };
        return std::bit_cast<T *>(std::bit_cast<std::byte *>(this) + offsetof(X, buf));
    }

    constexpr const T *inline_buffer() const
    {
        return const_cast<small_vector_base &>(*this).inline_buffer();
    }

    constexpr void __grow_uninitialized_exact(const size_type desired_capacity)
    {
        T *newp = allocate_buffer(desired_capacity);
        std::uninitialized_move(begin(), end(), newp);
        std::destroy_n(data(), n_);
        delete_buffer(data_);
        data_ = static_cast<void *>(newp);
        capacity_ = desired_capacity;
    }

    [[gnu::noinline]] constexpr void
    grow_uninitialized_exact(const size_type desired_capacity)
    {
        DEBUG_ASSERT(desired_capacity > capacity_);
        __grow_uninitialized_exact(desired_capacity);
    }

    [[gnu::noinline]] constexpr void
    grow_uninitialized_at_least(const size_type desired_capacity)
    {
        DEBUG_ASSERT(desired_capacity > capacity_);
        const size_t delta = desired_capacity - capacity_;
        const size_t shift = std::countr_zero(std::bit_ceil(delta));
        __grow_uninitialized_exact(capacity_ << shift);
    }

    [[gnu::noinline]] constexpr void grow_uninitialized_one_more()
    {
        __grow_uninitialized_exact(2 * capacity_);
    }

    constexpr iterator make_room_for_insertion(const_iterator pos, const size_type count)
    {
        const size_type i = pos - begin();

        if (n_ + count > capacity_) [[unlikely]]
            grow_uninitialized_at_least(n_ + count);

        iterator result = data() + i;
        std::move_backward(result, data() + n_, data() + n_ + count);
        n_ += count;
        return result;
    }

public:
    constexpr void assign(size_type count, const T &value)
    {
        clear();
        resize(count, value);
    }

    //-----------------------------------------------------------------------------------
    // Element access
    //-----------------------------------------------------------------------------------

    constexpr reference at(const size_type pos) noexcept
    {
        DEBUG_ASSERT(pos < n_);
        return static_cast<T *>(data_)[pos];
    }
    constexpr const_reference at(const size_type pos) const noexcept
    {
        DEBUG_ASSERT(pos < n_);
        return static_cast<T *>(data_)[pos];
    }
    constexpr reference operator[](const size_type pos) noexcept { return at(pos); }
    constexpr const_reference operator[](const size_type pos) const noexcept
    {
        return at(pos);
    }
    constexpr reference front() noexcept { return at(0); }
    constexpr const_reference front() const noexcept { return at(0); }
    constexpr reference back() noexcept { return at(n_ - 1); }
    constexpr const_reference back() const noexcept { return at(n_ - 1); }

    //-----------------------------------------------------------------------------------
    // Iterators
    //-----------------------------------------------------------------------------------

    constexpr T *data() noexcept { return static_cast<T *>(data_); }
    constexpr const T *data() const noexcept { return static_cast<T *>(data_); }

    constexpr iterator begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr const_iterator cbegin() const noexcept { return data(); }

    constexpr iterator end() noexcept { return data() + n_; }
    constexpr const_iterator end() const noexcept { return data() + n_; }
    constexpr const_iterator cend() const noexcept { return data() + n_; }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(data()); }
    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(data());
    }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    constexpr reverse_iterator rend() noexcept { return reverse_iterator(data() + n_); }
    constexpr const_reverse_iterator rend() const noexcept
    {
        return reverse_iterator(data() + n_);
    }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    //-----------------------------------------------------------------------------------
    // Capacity
    //-----------------------------------------------------------------------------------

    constexpr void reserve(size_type count)
    {
        if (count > capacity_)
            grow_uninitialized_exact(count);
    }

    //-----------------------------------------------------------------------------------
    // Modifiers
    //-----------------------------------------------------------------------------------

    constexpr void clear() noexcept(noexcept(std::is_nothrow_destructible_v<T>))
    {
        std::destroy_n(data(), n_);
        n_ = 0;
    }

    constexpr iterator insert(const_iterator pos, const T &value)
    {
        return emplace(pos, value);
    }

    constexpr iterator insert(const_iterator pos, T &&value)
    {
        return emplace(pos, static_cast<T &&>(value));
    }

    constexpr iterator insert(const_iterator pos, size_type count, const T &value)
    {
        DEBUG_ASSERT(pos >= begin());
        DEBUG_ASSERT(pos <= end());
        iterator result = make_room_for_insertion(pos, count);
        std::fill_n(result, count, value);
        return result;
    }

    // TODO: Support arbitrary input ranges, not just sized ones.
    template <std::ranges::sized_range R>
    constexpr iterator insert_range(const_iterator pos, R &&rg)
    {
        DEBUG_ASSERT(pos >= begin());
        DEBUG_ASSERT(pos <= end());
        const size_type count = std::ranges::size(rg);
        iterator result = make_room_for_insertion(pos, count);
        std::ranges::copy(rg, result);
        return result;
    }

    // TODO: Support arbitrary input iterators, not just sized ones.
    template <std::input_iterator I, std::sized_sentinel_for<I> S>
    constexpr iterator insert(const_iterator pos, I first, S last)
    {
        DEBUG_ASSERT(pos >= begin());
        DEBUG_ASSERT(pos <= end());
        const size_type count = last - first;
        iterator result = make_room_for_insertion(pos, count);
        std::copy_n(first, count, result);
        return result;
    }

    constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    constexpr iterator emplace(const_iterator pos, Args &&...args)
    {
        DEBUG_ASSERT(pos >= begin());
        DEBUG_ASSERT(pos <= end());

        const size_type i = pos - begin();

        if (n_ >= capacity_) [[unlikely]]
            grow_uninitialized_one_more();

        T *dst = data() + i;
        std::move_backward(dst, data() + n_, data() + n_ + 1);
        std::destroy_at(dst);
        std::construct_at(dst, static_cast<Args &&>(args)...);

        ++n_;
        return const_cast<iterator>(pos);
    }

    constexpr iterator erase(const_iterator pos) { return erase(pos, pos + 1); }

    constexpr iterator erase(const_iterator first, const_iterator last)
    {
        DEBUG_ASSERT(first <= last);
        DEBUG_ASSERT(first >= data());
        DEBUG_ASSERT(last <= data() + n_);
        auto dst = const_cast<iterator>(first);
        auto src = const_cast<iterator>(last);
        const size_type count = src - dst;
        std::move(src, end(), dst);
        std::destroy(end() - count, end());
        n_ -= count;
        return dst;
    }

    constexpr reference push_back(const T &value) { return emplace_back(value); }
    constexpr reference push_back(T &&value) { return emplace_back(std::move(value)); }

    template <class... Args>
    constexpr reference emplace_back(Args &&...args)
    {
        if (n_ >= capacity_) [[unlikely]]
            grow_uninitialized_one_more();

        T &result = data()[n_++];
        std::construct_at(std::addressof(result), static_cast<Args &&>(args)...);
        return result;
    }

    // TODO: Support arbitrary input ranges, not just sized ones.
    template <std::ranges::sized_range R>
    constexpr void append_range(R &&rg)
    {
        const size_type count = std::ranges::size(rg);

        if (n_ + count > capacity_) [[unlikely]]
            grow_uninitialized_at_least(n_ + count);

        std::ranges::copy(rg, end());
        n_ += count;
    }

    constexpr void pop_back() noexcept(noexcept(std::is_nothrow_destructible_v<T>))
    {
        DEBUG_ASSERT(n_);
        --n_;
        std::destroy_at(data() + n_);
    }

    constexpr void resize(const size_type count)
    {
        if (count > n_) {
            reserve(count);
            for (size_t i = n_; i < count; ++i)
                std::construct_at(data() + i);
        } else {
            std::destroy(data() + count, data() + n_);
        }
        n_ = count;
    }

    constexpr void resize(const size_type count, const T &value)
    {
        if (count > n_) {
            reserve(count);
            for (size_t i = n_; i < count; ++i)
                std::construct_at(data() + i, value);
        } else {
            std::destroy_n(data() + count, n_);
        }
        n_ = count;
    }

    constexpr void swap(small_vector_base &other) noexcept(noexcept(
        (std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)))
    {
        using std::swap;
        const bool on_heap = data() != inline_buffer();
        const bool other_on_heap = other.data() != other.inline_buffer();

        if (on_heap != other_on_heap) {
            // The elements in one vector (h) is allocated on the heap, while
            // the elements in the other vector (s) are located on the stack.
            small_vector_base &h = on_heap ? *this : other;
            small_vector_base &s = on_heap ? other : *this;
            std::uninitialized_move(s.data(), s.data() + s.n_, h.inline_buffer());
            std::destroy_n(s.data(), s.n_);
            s.data_ = std::exchange(h.data_, h.inline_buffer());
        } else if (on_heap) {
            // The elements of both vectors are located on the heap; all we
            // need to do is swap the pointers.
            swap(data_, other.data_);
        } else {
            // The elements of both vectors are inline.
            small_vector_base &a = n_ < other.n_ ? *this : other;
            small_vector_base &b = n_ < other.n_ ? other : *this;
            std::swap_ranges(a.data(), a.data() + a.n_, b.data());
            std::uninitialized_move(b.data() + a.n_, b.data() + b.n_, a.data() + a.n_);
            std::destroy(b.data() + a.n_, b.data() + b.n_);
        }

        swap(n_, other.n_);
        swap(capacity_, other.capacity_);
    }
};

template <typename T, size_t InlineCapacity = detail::default_inline_capacity<T>()>
class small_vector : public small_vector_base<T> {
    static_assert(InlineCapacity > 0);
    union {
        T inline_buffer_[InlineCapacity];
    };

public:
    using typename small_vector_base<T>::value_type;
    using typename small_vector_base<T>::size_type;
    using typename small_vector_base<T>::difference_type;
    using typename small_vector_base<T>::reference;
    using typename small_vector_base<T>::const_reference;
    using typename small_vector_base<T>::pointer;
    using typename small_vector_base<T>::const_pointer;
    using typename small_vector_base<T>::iterator;
    using typename small_vector_base<T>::const_iterator;
    using typename small_vector_base<T>::reverse_iterator;
    using typename small_vector_base<T>::const_reverse_iterator;

private:
    struct {
    } reserving_constructor_t;

    constexpr small_vector(decltype(reserving_constructor_t), const size_type n)
        : small_vector_base<T>(n > InlineCapacity ? this->allocate_buffer(n)
                                                  : inline_buffer_,
                               n,
                               std::max(n, InlineCapacity))
    {
    }

public:
    constexpr small_vector() noexcept
        : small_vector_base<T>(inline_buffer_, 0, InlineCapacity)
    {
    }

    constexpr small_vector(const size_type count) noexcept(
        noexcept(std::is_nothrow_constructible_v<T>))
        : small_vector(reserving_constructor_t, count)
    {
        for (size_type i = 0; i < count; ++i)
            std::construct_at(this->data() + i);
    }

    constexpr small_vector(const size_type count, const T &value) noexcept(
        noexcept(std::is_nothrow_copy_constructible_v<T>))
        : small_vector(reserving_constructor_t, count)
    {
        for (size_type i = 0; i < count; ++i)
            std::construct_at(this->data() + i, value);
    }

    template <std::input_iterator I, std::sized_sentinel_for<I> S>
    constexpr small_vector(I first, S last) noexcept(
        noexcept(std::is_nothrow_constructible_v<T, decltype(*first)>))
        : small_vector(reserving_constructor_t, last - first)
    {
        std::uninitialized_copy(first, last, this->data());
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr small_vector(I first, S last) noexcept(
        noexcept(std::is_nothrow_constructible_v<T, decltype(*first)>))
        : small_vector()
    {
        for (; first != last; ++first)
            this->push_back(*first);
    }

    constexpr small_vector(std::initializer_list<T> init) noexcept(
        noexcept(std::is_nothrow_constructible_v<T, decltype(*init.begin())>))
        : small_vector(init.begin(), init.end())
    {
    }

    constexpr small_vector(const small_vector &other) noexcept(
        noexcept(std::is_nothrow_copy_constructible_v<T>))
        : small_vector(other.begin(), other.end())
    {
    }

    constexpr small_vector(small_vector &&other) noexcept
        : small_vector_base<T>(inline_buffer_, 0, 0)
    {
        this->n_ = std::exchange(other.n_, 0);
        this->capacity_ = std::exchange(other.capacity_, InlineCapacity);

        if (other.data() != other.inline_buffer_) {
            this->data_ = std::exchange(other.data_, other.inline_buffer_);
        } else {
            std::uninitialized_move(other.inline_buffer_, other.inline_buffer_ + this->n_,
                                    this->data());
        }
    }

    constexpr small_vector &operator=(small_vector other)
    {
        // TODO: eh, this is suboptimal; we should preserve the already
        // allocated heap buffer here if we have one
        swap(*this, other);
        return *this;
    }

    constexpr ~small_vector() noexcept(noexcept(std::is_nothrow_destructible_v<T>)) {}
};

template <std::equality_comparable T>
constexpr bool operator==(const small_vector_base<T> &a,
                          const small_vector_base<T> &b) noexcept
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <std::three_way_comparable T>
constexpr auto operator<=>(const small_vector_base<T> &a,
                           const small_vector_base<T> &b) noexcept
{
    return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T>
constexpr void swap(small_vector_base<T> &a,
                    small_vector_base<T> &b) noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}

template <typename T, std::size_t N>
constexpr void swap(small_vector<T, N> &a,
                    small_vector<T, N> &b) noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}

template <class T>
constexpr size_t erase(small_vector_base<T> &c, const auto &value)
{
    const auto it = std::remove(c.begin(), c.end(), value);
    const size_t n = c.end() - it;
    c.erase(it, c.end());
    return n;
}

template <class T, class Predicate>
constexpr size_t erase_if(small_vector_base<T> &c, Predicate &&predicate)
{
    const auto it =
        std::remove_if(c.begin(), c.end(), static_cast<Predicate &&>(predicate));
    const size_t n = c.end() - it;
    c.erase(it, c.end());
    return n;
}
