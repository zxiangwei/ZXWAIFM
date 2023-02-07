#ifndef RPC_SERIALIZER_HPP_
#define RPC_SERIALIZER_HPP_

#include "rpc_buffer.hpp"
#include "rpc_coding.hpp"

#include <memory>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

namespace rpc {

class Serializer {
 public:
  Serializer() : buffer_(std::make_shared<rpc::Buffer>(64)) {}
  explicit Serializer(BufferPtr buffer) : buffer_(std::move(buffer)) {}

  BufferPtr GetBuffer() const { return buffer_; }

  /* Write functions */

  void WriteRaw(const char *data, size_t len) {
    buffer_->Append(data, len);
  }

  // bool, int, char, unsigned, etc
  template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
  void Write(T data) {
    buffer_->EnsureWritableBytes(sizeof(T));
    Encode(buffer_->GetWritePtr(), data);
    buffer_->HasWritten(sizeof(T));
  }

  template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
  void Write(T data) {
    Write(static_cast<std::underlying_type_t<T>>(data));
  }

  void Write(float data) {
    uint32_t idata;
    std::memcpy(&idata, &data, sizeof(float));
    Write(idata);
  }

  void Write(double data) {
    uint64_t idata;
    std::memcpy(&idata, &data, sizeof(double));
    Write(idata);
  }

  void Write(const char *str) {
    size_t len = std::strlen(str);
    Write(len);
    WriteRaw(str, len);
  }

  void Write(std::string_view s) {
    Write(s.size());
    WriteRaw(s.data(), s.size());
  }

  /* Read functions */
  size_t ReadableBytes() const { return buffer_->ReadableBytes(); }

  void ReadRaw(char *dst, size_t len) {
    std::memcpy(dst, buffer_->GetReadPtr(), len);
    buffer_->HasRead(len);
  }

  template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
  void Read(T &data) {
    Decode(buffer_->GetReadPtr(), data);
    buffer_->HasRead(sizeof(T));
  }

  template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
  void Read(T &data) {
    using UT = std::underlying_type_t<T>;
    UT ut;
    Read(ut);
    data = static_cast<T>(ut);
  }

  void Read(float &data) {
    uint32_t idata;
    Read(idata);
    std::memcpy(&data, &idata, sizeof(float));
  }

  void Read(double &data) {
    uint64_t idata;
    Read(idata);
    std::memcpy(&data, &idata, sizeof(double));
  }

  void Read(std::string &s) {
    size_t len;
    Read(len);
    s.resize(len);
    ReadRaw(s.data(), len);
  }

 private:
  BufferPtr buffer_;
};

/* Default serialize */
template<typename T>
Serializer &operator<<(Serializer &serializer, T data) {
  serializer.Write(data);
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, T &data) {
  serializer.Read(data);
  return serializer;
}

/* Helper Get function */
template<typename T>
T Get(Serializer &serializer) {
  T t{};
  serializer >> t;
  return t;
}

/* Serialize for BufferPtr */
inline Serializer &operator<<(Serializer &serializer, const BufferPtr &buffer) {
  serializer << buffer->ReadableBytes();
  serializer.WriteRaw(buffer->GetReadPtr(), buffer->ReadableBytes());
  return serializer;
}

inline Serializer &operator>>(Serializer &serializer, BufferPtr &buffer) {
  size_t len;
  serializer >> len;
  BufferPtr buf = std::make_shared<Buffer>(len);
  serializer.ReadRaw(buf->GetWritePtr(), len);
  buf->HasWritten(len);
  buffer = buf;
  return serializer;
}

/* Serialize for std::pair */
template<typename T1, typename T2>
Serializer &operator<<(Serializer &serializer, const std::pair<T1, T2> &p) {
  serializer << p.first;
  serializer << p.second;
  return serializer;
}

template<typename T1, typename T2>
Serializer &operator>>(Serializer &serializer, std::pair<T1, T2> &p) {
  serializer >> p.first;
  serializer >> p.second;
  return serializer;
}

/* Serialize for std::tuple */
template<typename ...Ts>
Serializer &operator<<(Serializer &serializer, const std::tuple<Ts...> &t) {
  std::apply([&serializer](Ts const &... args) {
    (void)(serializer << ... << args);
  }, t);
  return serializer;
}

template<typename ...Ts>
Serializer &operator>>(Serializer &serializer, std::tuple<Ts...> &t) {
  std::apply([&serializer](Ts &... args) {
    (void)(serializer >> ... >> args);
  }, t);
  return serializer;
}

/* Serialize for std::vector */
template<typename T>
Serializer &operator<<(Serializer &serializer, const std::vector<T> &vec) {
  serializer << vec.size();
  for (const auto &elem : vec) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::vector<T> &vec) {
  vec.resize(Get<size_t>(serializer));
  for (auto &elem : vec) {
    serializer >> elem;
  }
  return serializer;
}

/* Serialize for std::list */
template<typename T>
Serializer &operator<<(Serializer &serializer, const std::list<T> &lst) {
  serializer << lst.size();
  for (const auto &elem : lst) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::list<T> &lst) {
  lst.resize(Get<size_t>(serializer));
  for (auto &elem : lst) {
    serializer >> elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator<<(Serializer &serializer, const std::deque<T> &deq) {
  serializer << deq.size();
  for (const auto &elem : deq) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::deque<T> &deq) {
  deq.resize(Get<size_t>(serializer));
  for (auto &elem : deq) {
    serializer >> elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator<<(Serializer &serializer, const std::set<T> &s) {
  serializer << s.size();
  for (const auto &elem : s) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::set<T> &s) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    s.emplace(Get<T>(serializer));
  }
  return serializer;
}

template<typename T>
Serializer &operator<<(Serializer &serializer, const std::multiset<T> &s) {
  serializer << s.size();
  for (const auto &elem : s) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::multiset<T> &s) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    s.emplace(Get<T>(serializer));
  }
  return serializer;
}

template<typename T>
Serializer &operator<<(Serializer &serializer, const std::unordered_set<T> &s) {
  serializer << s.size();
  for (const auto &elem : s) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::unordered_set<T> &s) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    s.emplace(Get<T>(serializer));
  }
  return serializer;
}

template<typename T>
Serializer &operator<<(Serializer &serializer, const std::unordered_multiset<T> &s) {
  serializer << s.size();
  for (const auto &elem : s) {
    serializer << elem;
  }
  return serializer;
}

template<typename T>
Serializer &operator>>(Serializer &serializer, std::unordered_multiset<T> &s) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    s.emplace(Get<T>(serializer));
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator<<(Serializer &serializer, const std::map<Key, Value> &m) {
  serializer << m.size();
  for (const auto &elem : m) {
    serializer << elem;
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator>>(Serializer &serializer, std::map<Key, Value> &m) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    m.emplace(Get<std::pair<Key, Value>>(serializer));
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator<<(Serializer &serializer, const std::multimap<Key, Value> &m) {
  serializer << m.size();
  for (const auto &elem : m) {
    serializer << elem;
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator>>(Serializer &serializer, std::multimap<Key, Value> &m) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    m.emplace(Get<std::pair<Key, Value>>(serializer));
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator<<(Serializer &serializer, const std::unordered_map<Key, Value> &m) {
  serializer << m.size();
  for (const auto &elem : m) {
    serializer << elem;
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator>>(Serializer &serializer, std::unordered_map<Key, Value> &m) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    m.emplace(Get<std::pair<Key, Value>>(serializer));
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator<<(Serializer &serializer, const std::unordered_multimap<Key, Value> &m) {
  serializer << m.size();
  for (const auto &elem : m) {
    serializer << elem;
  }
  return serializer;
}

template<typename Key, typename Value>
Serializer &operator>>(Serializer &serializer, std::unordered_multimap<Key, Value> &m) {
  auto sz = Get<size_t>(serializer);
  for (int i = 0; i < sz; ++i) {
    m.emplace(Get<std::pair<Key, Value>>(serializer));
  }
  return serializer;
}

} // namespace rpc

#include "rpc_serialize_macro.hpp"

#endif
