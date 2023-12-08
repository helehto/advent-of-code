#ifndef DENSE_MAP_H
#define DENSE_MAP_H

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

template <typename Key, class T, class Hash = std::hash<Key>,
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
  // NOTE: The code is dependent on the states having these specific values.
  enum class bucket_state : uint8_t {
    empty = 0,
    occupied = 1,
    tombstone = 2,
    sentinel = 3,
  };

  struct bucket {
    alignas(value_type) char buffer[sizeof(value_type)];
    value_type &data() { return *reinterpret_cast<value_type *>(buffer); }
    const value_type &data() const { return *reinterpret_cast<const value_type *>(buffer); }
  };

public:
  class const_iterator {
    friend class dense_map;

    const_iterator(const dense_map *set, size_t index) : set_(set), index_(index) {}

    const dense_map *set_;
    size_t index_;

  public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<const Key, T>;
    using pointer = const value_type *;
    using reference = const value_type &;

    const_iterator() = default;

    bool operator==(const const_iterator &other) const { return index_ == other.index_; }
    bool operator!=(const const_iterator &other) const { return index_ != other.index_; }
    const value_type &operator*() const { return set_->buckets_[index_].data(); }
    const value_type *operator->() const { return &set_->buckets_[index_].data(); }
    const_iterator &operator++() { return void(index_ = set_->find_occupied_(index_ + 1)), *this; }
    const_iterator operator++(int) { return ++const_iterator(*this); }
  };

  class iterator {
    friend class dense_map;

    iterator(dense_map *set, size_t index) : set_(set), index_(index) {}

    dense_map *set_;
    size_t index_;

  public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<const Key, T>;
    using pointer = const value_type *;
    using reference = const value_type &;

    iterator() = default;

    operator const_iterator() { return const_iterator(set_, index_); }

    bool operator==(const iterator &other) const { return index_ == other.index_; }
    bool operator!=(const iterator &other) const { return index_ != other.index_; }
    value_type &operator*() { return set_->buckets_[index_].data(); }
    value_type *operator->() { return &set_->buckets_[index_].data(); }
    const value_type &operator*() const { return set_->buckets_[index_].data(); }
    const value_type *operator->() const { return &set_->buckets_[index_].data(); }
    iterator &operator++() { return void(index_ = set_->find_occupied_(index_ + 1)), *this; }
    iterator operator++(int) { return ++iterator(*this); }
  };

private:
  std::unique_ptr<bucket[]> buckets_;
  std::unique_ptr<uint8_t[]> states_;  // TODO: pack?
  size_t capacity_;
  size_t mask_;
  size_t size_;
  size_t size_with_tombs_;
  hasher hash_;
  key_equal equal_;

  iterator iterator_from_index(size_t i) { return iterator(this, i); }
  const_iterator iterator_from_index(size_t i) const { return const_iterator(this, i); }

  bucket_state state_of(size_t i) const {
    return static_cast<bucket_state>(states_[i]);
  }

  void set_state_of(size_t i, bucket_state state) {
    states_[i] = static_cast<uint8_t>(state);
  }

  size_t find_occupied_(size_t i) const {
    for (;; ++i)
      if (static_cast<int>(state_of(i)) & 1)
        return i;
  }

  std::pair<size_t, bool> find_bucket_(const Key &key) const {
    const size_t hash = hash_(key);
    const auto mask = mask_;
    size_t i = hash & mask;

    while (1) {
      switch (state_of(i)) {
      case bucket_state::occupied:
        if (equal_(buckets_[i].data().first, key))
          return {i, true};
        break;

      case bucket_state::empty:
        return {i, false};

      case bucket_state::tombstone:
      case bucket_state::sentinel:
        break;
      }
     i = (i + 1) & mask;
    }
  }

  constexpr static auto max_load_ = std::make_pair(3, 4);

  template <typename ConstructFn>
  std::pair<iterator, bool> do_insert_helper_(const key_type &key, ConstructFn &&construct) {
    const auto [i, found] = find_bucket_(key);
    if (!found) {
      construct(buckets_[i].buffer);
      set_state_of(i, bucket_state::occupied);
      size_++;
      size_with_tombs_++;

      if (max_load_.second * size_with_tombs_ >= capacity_ * max_load_.first) {
        rehash(2 * capacity_);
        return {iterator_from_index(find_bucket_(key).first), false};
      }
    }
    return {iterator_from_index(i), !found};
  }

  std::pair<iterator, bool> do_insert_(const value_type &value) {
    return do_insert_helper_(value.first, [&value](void *buffer) {
      new (buffer) value_type(value);
    });
  }

  std::pair<iterator, bool> do_insert_(value_type &&value) {
    return do_insert_helper_(value.first, [&value](void *buffer) {
      new (buffer) value_type(std::move(value));
    });
  }

public:
  //-------------------------------------------------------------------------
  // Construction and destruction.
  //-------------------------------------------------------------------------

  dense_map() : dense_map(size_type(16)) {}

  dense_map(size_type bucket_count, const Hash &hash = Hash(), const KeyEqual &equal = KeyEqual())
      : buckets_(std::make_unique<bucket[]>(bucket_count + 1))
      , states_(std::make_unique<uint8_t[]>(bucket_count + 1))
      , capacity_(bucket_count)
      , mask_((1UL << __builtin_ctzl(capacity_)) - 1)
      , size_(0)
      , size_with_tombs_(0)
      , hash_(hash)
      , equal_(equal) {
    assert((bucket_count & (bucket_count - 1)) == 0);
    memset(&states_[0], 0, sizeof(bucket_state) * bucket_count);
    set_state_of(capacity_, bucket_state::sentinel);
  }

  // TODO: Better bucket count, based on the capacity other hash table?
  dense_map(const dense_map &other)
      : dense_map(other.begin(), other.end(), 16, other.hash_, other.equal_) {}

  // TODO.
  dense_map &operator=(const dense_map &) = delete;

  dense_map(dense_map &&other)
      : buckets_(std::exchange(other.buckets_, nullptr))
      , states_(std::exchange(other.states_, nullptr))
      , capacity_(std::exchange(other.capacity_, 0))
      , size_(std::exchange(other.size_, 0))
      , size_with_tombs_(std::exchange(other.size_with_tombs_, 0))
      , hash_(std::exchange(other.hash_, Hash{}))
      , equal_(std::exchange(other.equal_, KeyEqual{})) {}

  // TODO.
  dense_map &operator=(dense_map &&) = delete;

  template <typename InputIt>
  dense_map(InputIt begin, InputIt end, size_type bucket_count = 16, const Hash &hash = Hash(),
            const KeyEqual &equal = KeyEqual())
      : dense_map(bucket_count, hash, equal) {
    for (; begin != end; ++begin)
      insert(*begin);
  }

  dense_map(std::initializer_list<value_type> list, size_type bucket_count = 16,
            const Hash &hash = Hash(), const KeyEqual &equal = KeyEqual())
      : dense_map(list.begin(), list.end(), bucket_count, hash, equal) {}

  ~dense_map() {
    // TODO: Better way to iterate all occupied buckets?
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      for (size_t i = 0; i < capacity_; ++i) {
        if (state_of(i) == bucket_state::occupied)
          buckets_[i].data().~value_type();
      }
    }
  }

  //-------------------------------------------------------------------------
  // Iterators.
  // TODO: Less stupid way to find the first element
  //-------------------------------------------------------------------------

  iterator begin() noexcept { return iterator_from_index(find_occupied_(0)); }
  const_iterator begin() const noexcept { return iterator_from_index(find_occupied_(0)); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return iterator_from_index(capacity_); }
  const_iterator end() const noexcept { return iterator_from_index(capacity_); }
  const_iterator cend() const noexcept { return iterator_from_index(capacity_); }

  //-------------------------------------------------------------------------
  // Capacity.
  //-------------------------------------------------------------------------

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  size_type size() const noexcept { return size_; }

  //-------------------------------------------------------------------------
  // Modifiers.
  //-------------------------------------------------------------------------

  void clear() noexcept {
    // TODO: Better way to iterate all occupied buckets?
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
        for (size_t i = 0; i < capacity_; ++i) {
          if (state_of(i) == bucket_state::occupied)
            buckets_[i].data().~value_type();
        }
    }
    memset(states_.get(), 0, capacity_);

    size_ = 0;
    size_with_tombs_ = 0;
  }

  std::pair<iterator, bool> insert(const value_type &value) { return do_insert_(value); }
  std::pair<iterator, bool> insert(value_type &&value) { return do_insert_(std::move(value)); }

  // insert() with insertion hints. The hints are just ignored, for now.
  std::pair<iterator, bool> insert(const_iterator, const value_type &value) {
    return do_insert_(value);
  }
  std::pair<iterator, bool> insert(const_iterator, value_type &&value) {
    return do_insert_(std::move(value));
  }

  template <typename InputIt> void insert(InputIt begin, InputIt end) {
    for (; begin != end; ++begin)
      do_insert_(*begin);
  }

  void insert(std::initializer_list<value_type> list) { insert(list.begin(), list.end()); }

  template <typename... Args> std::pair<iterator, bool> emplace(Args &&... args) {
    return do_insert_(value_type(std::forward<Args>(args)...));
  }

  // emplace() with insertion hints. As with insert(), the hint is ignored.
  template <typename... Args>
  std::pair<iterator, bool> emplace_hint(const_iterator, Args &&... args) {
    return do_insert_(value_type(std::forward<Args>(args)...));
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(const key_type &k, Args &&... args) {
    return do_insert_helper_(k, [&](void *buffer) {
      new (buffer) value_type(std::piecewise_construct, std::forward_as_tuple(k),
                              std::forward_as_tuple(std::forward<Args>(args)...));
    });
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type &&k, Args &&... args) {
    return do_insert_helper_(k, [&](void *buffer) {
      new (buffer) value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)),
                              std::forward_as_tuple(std::forward<Args>(args)...));
    });
  }

  template <class... Args>
  iterator try_emplace(const_iterator, const key_type& k, Args&&... args) {
    return try_emplace(k, std::forward<Args>(args)...);
  }

  template <class... Args>
  iterator try_emplace(const_iterator, key_type&& k, Args&&... args) {
    return try_emplace(std::move(k), std::forward<Args>(args)...);
  }

  iterator erase(const_iterator pos) {
    assert(pos.set_ == this);
    assert(state_of(pos.index_) == bucket_state::occupied);
    buckets_[pos.index_].data().~value_type();
    set_state_of(pos.index_, bucket_state::tombstone);
    size_--;

    // Find the next occupied bucket.
    return iterator_from_index(find_occupied_(pos.index_ + 1));
  }

  size_type erase(const key_type &key) {
    if (const auto [i, found] = find_bucket_(key); found) {
      buckets_[i].data().~value_type();
      set_state_of(i, bucket_state::tombstone);
      size_--;
      return 1;
    }
    return 0;
  }

  void swap(dense_map &other) {
    using std::swap;
    swap(buckets_, other.buckets_);
    swap(states_, other.states_);
    swap(capacity_, other.capacity_);
    swap(mask_, other.mask_);
    swap(size_, other.size_);
    swap(size_with_tombs_, other.size_with_tombs_);
    swap(hash_, other.hash_);
    swap(equal_, other.equal_);
  }

  //-------------------------------------------------------------------------
  // Lookup.
  //-------------------------------------------------------------------------

  T &at(const key_type &key) {
    if (const auto [i, found] = find_bucket_(key); found)
      return buckets_[i].data().second;
    abort();
  }

  const T &at(const key_type &key) const {
    if (const auto [i, found] = find_bucket_(key); found)
      return buckets_[i].data().second;
    abort();
  }

  size_type count(const key_type &key) const {
    const auto [i, found] = find_bucket_(key);
    return found ? 1 : 0;
  }

  iterator find(const key_type &key) {
    const auto [i, found] = find_bucket_(key);
    return found ? iterator_from_index(i) : end();
  }

  const_iterator find(const key_type &key) const {
    const auto [i, found] = find_bucket_(key);
    return found ? iterator_from_index(i) : end();
  }

  T &operator[](const key_type &key) { return try_emplace(key).first->second; }
  T &operator[](key_type &&key) { return try_emplace(std::move(key)).first->second; }

  //-------------------------------------------------------------------------
  // Hash policy.
  //-------------------------------------------------------------------------

  float load_factor() const noexcept { return static_cast<float>(size_with_tombs_) / capacity_; }
  float max_load_factor() const noexcept { return static_cast<float>(max_load_.first) / max_load_.second; }

  void rehash(size_type count) {
    dense_map new_set(count, hash_, equal_);
    for (auto &elem : *this)
      new_set.insert(std::move(elem));
    swap(new_set);
  }

  void reserve(size_type count) {
    size_t c = std::ceil(count / max_load_factor());
    c |= c >> 1;
    c |= c >> 2;
    c |= c >> 4;
    c |= c >> 8;
    c |= c >> 16;
    c |= c >> 32;
    rehash(c + 1);
  }

  //-------------------------------------------------------------------------
  // Observers.
  //-------------------------------------------------------------------------
  hasher hash_function() const { return hash_; }
  key_equal key_eq() const { return equal_; }
};

namespace std {
template <typename Key, class Hash, class KeyEqual>
void swap(dense_map<Key, Hash, KeyEqual> &a, dense_map<Key, Hash, KeyEqual> &b) {
  a.swap(b);
}
}  // namespace std

#endif /* DENSE_MAP_H */
