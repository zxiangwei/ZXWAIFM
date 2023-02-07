#ifndef RPC_ROUTER_HPP_
#define RPC_ROUTER_HPP_

#include "rpc_traits.hpp"
#include "rpc_serializer.hpp"
#include "rpc_protocol.hpp"

#include <string>
#include <unordered_map>

namespace rpc {

class RpcRouter {
 public:
  using WrappedFunction = std::function<void(const BufferPtr &args, BufferPtr &ret)>;

  RpcRouter() = default;

  template<typename F>
  void Register(std::string name, F func) {
    name_to_function_.emplace(std::move(name), [func](const BufferPtr &args, BufferPtr &ret) {
      using ReturnType = typename FunctionTraits<F>::ReturnType;
      using TupleArgType = typename FunctionTraits<F>::TupleArgType;
      Serializer serializer(args);
      auto tuple_arg = Get<TupleArgType>(serializer);

      if constexpr(std::is_void_v<ReturnType>) {
        std::apply(func, tuple_arg);
        ret = std::make_shared<Buffer>(0);
      } else {
        ReturnType ret_value = std::apply(func, tuple_arg);
        Serializer out;
        out << ret_value;
        ret = out.GetBuffer();
      }
    });
  }

  [[nodiscard]] RpcReplyBody Call(const std::string &method, const BufferPtr &args) {
    RpcReplyBody response;
    if (name_to_function_.count(method) == 0) {
      response.error_code = RpcErrorCode::kMethodNotFound;
      response.ret = std::make_shared<Buffer>(0);
      return response;
    }
    WrappedFunction &func = name_to_function_[method];
    func(args, response.ret);
    response.error_code = RpcErrorCode::kSuccess;
    return response;
  }

  // 在服务器运行的时候Method是不变的，因此不考虑线程安全的问题
  std::vector<std::string> GetAllMethods() {
    std::vector<std::string> methods;
    methods.reserve(name_to_function_.size());
    for (const auto &[name, f] : name_to_function_) {
      methods.emplace_back(name);
    }
    return methods;
  }

 private:
  std::unordered_map<std::string, WrappedFunction> name_to_function_;
};

}

#endif
