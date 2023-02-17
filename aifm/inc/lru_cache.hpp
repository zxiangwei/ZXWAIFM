#pragma once

#include <list>
#include <unordered_map>

template<typename Key, typename Value>
class LRUCache {
  static constexpr bool kDisabled = true;
 public:

  explicit LRUCache(size_t capacity) : capacity_(capacity) {
  }

  bool has(const Key &key) {
    if constexpr(kDisabled) {
      return false;
    }
    return key_to_iter_.count(key);
  }

  // 必须确保has(key)为true
  const Value &get(const Key &key) {
    auto iter = key_to_iter_[key];
    kv_list_.splice(kv_list_.end(), kv_list_, iter);
    return iter->second;
  }

  // 必须确保has(key)为true
  void remove(const Key &key) {
    auto iter = key_to_iter_[key];
    kv_list_.erase(iter);
    key_to_iter_.erase(key);
  }

  void set(const Key &key, const Value &value) {
    if constexpr(kDisabled) {
      return;
    }
    if (key_to_iter_.count(key)) {
      auto iter = key_to_iter_[key];
      iter->second = value;
      kv_list_.splice(kv_list_.end(), kv_list_, iter);
      return;
    }
    if (kv_list_.size() == capacity_) {
      key_to_iter_.erase(kv_list_.front().first);
      kv_list_.pop_front();
    }
    key_to_iter_[key] = kv_list_.insert(kv_list_.end(), std::make_pair(key, value));
  }
 private:
  size_t capacity_;
  std::list<std::pair<Key, Value>> kv_list_;
  std::unordered_map<Key, typename decltype(kv_list_)::iterator> key_to_iter_;
};

