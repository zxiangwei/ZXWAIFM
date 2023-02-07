#ifndef RPC_CODING_HPP_
#define RPC_CODING_HPP_

#include <cstring>
#include <cstdint>
#include <type_traits>

namespace rpc {

namespace {

uint32_t ZigZagEncode32(int32_t v) {
  return static_cast<uint32_t>((v << 1) ^ (v >> 31));
}

int32_t ZigZagDecode32(uint32_t v) {
  return static_cast<int32_t>(((v >> 1) ^ -(v & 1)));
}

uint64_t ZigZagEncode64(int64_t v) {
  return static_cast<uint64_t>((v << 1) ^ (v >> 63));
}

int64_t ZigZagDecode64(uint64_t v) {
  return static_cast<int64_t>((v >> 1) ^ -(v & 1));
}

}

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void Encode(char *dst, T value) {
  uint8_t *const buf = reinterpret_cast<uint8_t *>(dst);
  static_assert(sizeof(T) <= sizeof(uint64_t));
  if constexpr(sizeof(T) == sizeof(uint8_t)) {
    buf[0] = static_cast<uint8_t>(value);
  } else if constexpr(sizeof(T) == sizeof(uint16_t)) {
    buf[0] = static_cast<uint8_t>(value >> 8);
    buf[1] = static_cast<uint8_t>(value);
  } else if constexpr(sizeof(T) == sizeof(uint32_t)) {
    buf[0] = static_cast<uint8_t>(value >> 24);
    buf[1] = static_cast<uint8_t>(value >> 16);
    buf[2] = static_cast<uint8_t>(value >> 8);
    buf[3] = static_cast<uint8_t>(value);
  } else if constexpr(sizeof(T) == sizeof(uint64_t)) {
    buf[0] = static_cast<uint8_t>(value >> 56);
    buf[1] = static_cast<uint8_t>(value >> 48);
    buf[2] = static_cast<uint8_t>(value >> 40);
    buf[3] = static_cast<uint8_t>(value >> 32);
    buf[4] = static_cast<uint8_t>(value >> 24);
    buf[5] = static_cast<uint8_t>(value >> 16);
    buf[6] = static_cast<uint8_t>(value >> 8);
    buf[7] = static_cast<uint8_t>(value);
  }
}

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void Decode(const char *src, T &value) {
  const uint8_t *const buf = reinterpret_cast<const uint8_t *>(src);
  static_assert(sizeof(T) <= sizeof(uint64_t));
  if constexpr(sizeof(T) == sizeof(uint8_t)) {
    value = buf[0];
  } else if constexpr(sizeof(T) == sizeof(uint16_t)) {
    value = (static_cast<uint16_t>(buf[0]) << 8) |
        (static_cast<uint16_t>(buf[1]));
  } else if constexpr(sizeof(T) == sizeof(uint32_t)) {
    value = (static_cast<uint32_t>(buf[0]) << 24) |
        (static_cast<uint32_t>(buf[1]) << 16) |
        (static_cast<uint32_t>(buf[2]) << 8) |
        (static_cast<uint32_t>(buf[3]));
  } else if constexpr(sizeof(T) == sizeof(uint64_t)) {
    value = (static_cast<uint64_t>(buf[0]) << 56) |
        (static_cast<uint64_t>(buf[1]) << 48) |
        (static_cast<uint64_t>(buf[2]) << 40) |
        (static_cast<uint64_t>(buf[3]) << 32) |
        (static_cast<uint64_t>(buf[4]) << 24) |
        (static_cast<uint64_t>(buf[5]) << 16) |
        (static_cast<uint64_t>(buf[6]) << 8) |
        (static_cast<uint64_t>(buf[7]));
  }
}

inline int EncodeVarUint32(char *dst, uint32_t value) {
  uint8_t *const buf = reinterpret_cast<uint8_t *>(dst);
  int i = 0;
  while (value >= 0x80) {
    buf[i++] = (value & 0x7F) | (0x80);
    value >>= 7;
  }
  buf[i] = static_cast<uint8_t>(value);
  return i + 1;
}

inline int DecodeVarUint32(const char *src, uint32_t &value) {
  const uint8_t *const buf = reinterpret_cast<const uint8_t *>(src);
  int i = 0;
  while (buf[i] >= 0x80) {
    value += (buf[i++] & 0x7F);
    value <<= 7;
  }
  value += buf[i];
  return i + 1;
}

inline int EncodeVarUint64(char *dst, uint64_t value) {
  uint8_t *const buf = reinterpret_cast<uint8_t *>(dst);
  int i = 0;
  while (value >= 0x80) {
    buf[i++] = (value & 0x7F) | (0x80);
    value >>= 7;
  }
  buf[i] = static_cast<uint8_t>(value);
  return i + 1;
}

inline int DecodeVarUint64(const char *src, uint64_t &value) {
  const uint8_t *const buf = reinterpret_cast<const uint8_t *>(src);
  int i = 0;
  while (buf[i] >= 0x80) {
    value += (buf[i++] & 0x7F);
    value <<= 7;
  }
  value += buf[i];
  return i + 1;
}

inline int EncodeVarInt32(char *dst, int32_t value) {
  return EncodeVarUint32(dst, ZigZagEncode32(value));
}

inline int DecodeVarInt32(const char *src, int32_t &value) {
  uint32_t uv;
  int n = DecodeVarUint32(src, uv);
  value = ZigZagDecode32(uv);
  return n;
}

inline int EncodeVarInt64(char *dst, int64_t value) {
  return EncodeVarUint64(dst, ZigZagEncode64(value));
}

inline int DecodeVarInt64(const char *src, int64_t &value) {
  uint64_t uv;
  int n = DecodeVarUint64(src, uv);
  value = ZigZagDecode64(uv);
  return n;
}

} // namespace rpc

#endif
