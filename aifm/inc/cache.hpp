#pragma once

#include <cassert>
#include <memory>

#include "cache_impl.hpp"
#include "lru_cache.hpp"

template<typename Key, typename Value>
class Cache {
  static constexpr bool kDisabled = false;
  using Impl = detail::CacheImpl<Key, Value>;
 public:
  static Cache LRU(size_t capacity) {
    return Cache(std::make_shared<detail::LRUCache<Key, Value>>(capacity));
  }

  bool has(const Key &key) {
    if constexpr(kDisabled) {
      return false;
    }
    return impl_->has(key);
  }
  const Value &get(const Key &key) {
    if constexpr(kDisabled) {
      assert(false);
    }
    return impl_->get(key);
  }
  void remove(const Key &key) {
    if constexpr(kDisabled) {
      return;
    }
    return impl_->remove(key);
  }
  void set(const Key &key, const Value &value) {
    if constexpr(kDisabled) {
      return;
    }
    return impl_->set(key, value);
  }

 private:
  explicit Cache(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
  std::shared_ptr<Impl> impl_;
};
