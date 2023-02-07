#ifndef RPC_PROTOCOL_HPP_
#define RPC_PROTOCOL_HPP_

#include "rpc_serializer.hpp"

#include <cstdint>
#include <string>

namespace rpc {

// kRpcRequest的content部分
struct RpcRequestBody {
  std::string method;
  BufferPtr args; // 相比使用std::string来说，可以减少一次复制
};
SERIALIZE2(RpcRequestBody, method, args)

enum class RpcErrorCode {
  kSuccess,
  kMethodNotFound,
};

// kRpcReply的content部分
struct RpcReplyBody {
  RpcErrorCode error_code;
  BufferPtr ret;
};
SERIALIZE2(RpcReplyBody, error_code, ret)

template<typename ...Args>
BufferPtr SerializeArgsToBuffer(Args &&...args) {
  if constexpr(sizeof...(Args) == 0) {  // 针对无参数场景的优化
    return std::make_shared<Buffer>(0);
  } else {
    Serializer serializer;
    (void) (serializer << ... << args);
    return serializer.GetBuffer();
  }
}

template<typename T>
T GetReturnValueFromBuffer(const BufferPtr &buffer) {
  Serializer serializer(buffer);
  T val = Get<T>(serializer);
  return val;
}

}

#endif
