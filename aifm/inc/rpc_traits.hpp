#ifndef RPC_TRAITS_HPP_
#define RPC_TRAITS_HPP_

#include <functional>

namespace rpc {

template<typename Ret, typename ...Args>
struct FunctionTraitsBase {
  using ReturnType = Ret;
  static constexpr size_t kArgNum = sizeof...(Args);
  using TupleArgType = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
  using FunctionType = Ret(Args...);

  template<size_t Index>
  using ArgType = std::tuple_element_t<Index, TupleArgType>;
};

template<typename T>
struct FunctionTraits;

// 普通函数
template<typename Ret, typename ...Args>
struct FunctionTraits<Ret(Args...)> : FunctionTraitsBase<Ret, Args...> {};

// 普通函数指针
template<typename Ret, typename ...Args>
struct FunctionTraits<Ret(*)(Args...)> : FunctionTraitsBase<Ret, Args...> {};

// std::function
template<typename Ret, typename ...Args>
struct FunctionTraits<std::function<Ret(Args...)>> : FunctionTraitsBase<Ret, Args...> {};

// lambda
template<typename Ret, typename Class, typename ...Args>
struct FunctionTraits<Ret(Class::*)(Args...)> : FunctionTraitsBase<Ret, Args...> {};

template<typename Ret, typename Class, typename ...Args>
struct FunctionTraits<Ret(Class::*)(Args...) const> : FunctionTraitsBase<Ret, Args...> {};

// 可调用对象
template<typename Callable>
struct FunctionTraits : FunctionTraits<decltype(&Callable::operator())> {};

namespace detail {

inline void NormalFunc(int, int *, char, bool) {}

static_assert(std::is_same_v<FunctionTraits<decltype(NormalFunc)>::ReturnType, void>);
static_assert(std::is_same_v<FunctionTraits<decltype(NormalFunc)>::TupleArgType, std::tuple<int, int *, char, bool>>);

static_assert(std::is_same_v<FunctionTraits<decltype(&NormalFunc)>::ReturnType, void>);
static_assert(std::is_same_v<FunctionTraits<decltype(&NormalFunc)>::TupleArgType, std::tuple<int, int *, char, bool>>);
static_assert(std::is_same_v<FunctionTraits<decltype(&NormalFunc)>::ArgType<0>, int>);
static_assert(std::is_same_v<FunctionTraits<decltype(&NormalFunc)>::ArgType<1>, int *>);
static_assert(FunctionTraits<decltype(&NormalFunc)>::kArgNum == 4);

static_assert(std::is_same_v<FunctionTraits<std::function<bool(char)>>::ReturnType, bool>);
static_assert(std::is_same_v<FunctionTraits<std::function<bool(char)>>::TupleArgType, std::tuple<char>>);

auto lambda_func = [](char, bool) -> int { return 1; };

static_assert(std::is_same_v<FunctionTraits<decltype(lambda_func)>::ReturnType, int>);
static_assert(std::is_same_v<FunctionTraits<decltype(lambda_func)>::TupleArgType, std::tuple<char, bool>>);

struct ObjectFunc {
  char operator()(bool) { return 'a'; }
};

static_assert(std::is_same_v<FunctionTraits<ObjectFunc>::ReturnType, char>);
static_assert(std::is_same_v<FunctionTraits<ObjectFunc>::TupleArgType, std::tuple<bool>>);
static_assert(std::is_same_v<FunctionTraits<ObjectFunc>::FunctionType, char(bool)>);

}

}

#endif
