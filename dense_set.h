#ifndef DENSE_SET_H
#define DENSE_SET_H

#include "dense_map.h"

struct dense_set_key {};

template <typename Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class dense_set {
private:
    using underlying_map_type = dense_map<Key, dense_set_key, Hash, KeyEqual>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = const value_type &;
    using const_reference = const value_type &;

public:
    class const_iterator {
        friend class dense_set;

        const_iterator(typename underlying_map_type::iterator iter)
            : iter_(iter)
        {
        }

        const_iterator(typename underlying_map_type::const_iterator iter)
            : iter_(iter)
        {
        }

        typename underlying_map_type::const_iterator iter_;

    public:
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using value_type = dense_set::key_type;
        using pointer = const value_type *;
        using reference = const value_type &;

        const_iterator() = default;

        bool operator==(const const_iterator &other) const { return iter_ == other.iter_; }
        bool operator!=(const const_iterator &other) const { return iter_ != other.iter_; }
        reference operator*() const { return iter_->first; }
        pointer operator->() const { return &iter_->first; }
        const_iterator &operator++() { return void(++iter_), *this; }
        const_iterator operator++(int) { return ++*this; }
    };

    using iterator = const_iterator;

private:
    underlying_map_type map_;

public:
    //-------------------------------------------------------------------------
    // Construction and destruction.
    //-------------------------------------------------------------------------

    dense_set() = default;

    dense_set(size_type bucket_count,
              const Hash &hash = Hash(),
              const KeyEqual &equal = KeyEqual())
        : map_(bucket_count, hash, equal)
    {
    }

    template <typename InputIt>
    dense_set(InputIt begin,
              InputIt end,
              size_type bucket_count = 0,
              const Hash &hash = Hash(),
              const KeyEqual &equal = KeyEqual())
    {
        size_type map_bucket_count = bucket_count;
        using category = typename std::iterator_traits<InputIt>::iterator_category;
        if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) {
            map_bucket_count = std::distance(begin, end);
        }

        underlying_map_type map(map_bucket_count, hash, equal);
        for (; begin != end; ++begin)
            map.emplace(std::move(*begin), dense_set_key{});
        map_.swap(map);
    }

    dense_set(std::initializer_list<Key> list,
              size_type bucket_count = 0,
              const Hash &hash = Hash(),
              const KeyEqual &equal = KeyEqual())
        : map_(list.begin(), list.end(), std::max(bucket_count, list.size()), hash, equal)
    {
    }

    //-------------------------------------------------------------------------
    // Iterators.
    //-------------------------------------------------------------------------

    iterator begin() noexcept { return map_.begin(); }
    const_iterator begin() const noexcept { return map_.begin(); }
    const_iterator cbegin() const noexcept { return map_.cbegin(); }
    iterator end() noexcept { return map_.end(); }
    const_iterator end() const noexcept { return map_.end(); }
    const_iterator cend() const noexcept { return map_.cend(); }

    //-------------------------------------------------------------------------
    // Capacity.
    //-------------------------------------------------------------------------

    [[nodiscard]] bool empty() const noexcept { return map_.empty(); }
    size_type size() const noexcept { return map_.size(); }

    //-------------------------------------------------------------------------
    // Modifiers.
    //-------------------------------------------------------------------------

    void clear() noexcept { map_.clear(); }

    std::pair<iterator, bool> insert(const value_type &value)
    {
        auto [u, inserted] = map_.insert(std::make_pair(value, dense_set_key{}));
        return {iterator{u}, inserted};
    }
    std::pair<iterator, bool> insert(value_type &&value)
    {
        auto [u, inserted] = map_.insert(std::make_pair(std::move(value), dense_set_key{}));
        return {iterator{u}, inserted};
    }

    // insert() with insertion hints. The hints are just ignored, for now.
    std::pair<iterator, bool> insert(const_iterator, const value_type &value)
    {
        return insert(value);
    }
    std::pair<iterator, bool> insert(const_iterator, value_type &&value)
    {
        return insert(std::move(value));
    }

    template <typename InputIt>
    void insert(InputIt begin, InputIt end)
    {
        for (auto it = begin; it != end; ++it)
            insert(*it);
    }

    void insert(std::initializer_list<value_type> list) { map_.insert(list); }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args)
    {
        auto [it, b] = map_.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(std::forward<Args>(args)...),
                                    std::tuple<>());
        return {it, b};
    }

    // emplace() with insertion hints. As with insert(), the hint is ignored.
    template <typename... Args>
    std::pair<iterator, bool> emplace_hint(const_iterator, Args &&...args)
    {
        auto [it, b] = map_.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(std::forward<Args>(args)...),
                                    std::tuple<>());
        return {it, b};
    }

    iterator erase(const_iterator pos) { return map_.erase(pos.iter_); }
    size_type erase(const key_type &key) { return map_.erase(key); }

    void swap(dense_set &other) { map_.swap(other.map_); }

    //-------------------------------------------------------------------------
    // Lookup.
    //-------------------------------------------------------------------------

    size_type count(const key_type &key) const { return map_.count(key); }
    iterator find(const key_type &key) { return map_.find(key); }
    const_iterator find(const key_type &key) const { return map_.find(key); }

    //-------------------------------------------------------------------------
    // Hash policy.
    //-------------------------------------------------------------------------

    float load_factor() const noexcept { return map_.load_factor(); }
    float max_load_factor() const noexcept { return map_.max_load_factor(); }
    void rehash(size_type count) { return map_.rehash(count); }
    void reserve(size_type count) { return map_.reserve(count); }

    //-------------------------------------------------------------------------
    // Observers.
    //-------------------------------------------------------------------------
    hasher hash_function() const { return map_.hash_function(); }
    key_equal key_eq() const { return map_.key_eq(); }
};

namespace std {
template <typename Key, class Hash, class KeyEqual>
void swap(dense_set<Key, Hash, KeyEqual> &a, dense_set<Key, Hash, KeyEqual> &b)
{
    a.swap(b);
}
} // namespace std

#endif /* DENSE_SET_H */
