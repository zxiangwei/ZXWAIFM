#pragma once

namespace detail {

template<typename Key, typename Value>
class CacheImpl {
 public:
  virtual ~CacheImpl() = default;

  virtual bool has(const Key &key) = 0;
  virtual const Value &get(const Key &key) = 0;
  virtual void remove(const Key &key) = 0;
  virtual void set(const Key &key, const Value &value) = 0;
};

}
