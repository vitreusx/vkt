#pragma once
#include <utility>
#include <memory>
#include <type_traits>
#include <istream>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <sstream>
#include <iostream>
#include <functional>

template <typename _Res, typename... _ArgTypes>
class ICallback {
public:
  virtual _Res impl(_ArgTypes... args) = 0;
};

template <typename Func, typename _Res, typename... _ArgTypes>
class LambdaCb : public ICallback<_Res, _ArgTypes...> {
public:
  LambdaCb(Func func) : func{std::move(func)} {}

  _Res impl(_ArgTypes... args) {
    if constexpr (std::is_void_v<_Res>)
      func(std::forward<_ArgTypes>(args)...);
    else
      return func(std::forward<_ArgTypes>(args)...);
  }

private:
  Func func;
};

template <typename Sig>
class Callback;

template <typename _Res, typename... _ArgTypes>
class Callback<_Res (*)(_ArgTypes...)> {
public:
  Callback() = default;

  template <typename Func>
  Callback(Func func)
      : func{std::make_shared<LambdaCb<Func, _Res, _ArgTypes...>>(
            std::move(func))} {}

  template <typename Func>
  Callback &operator=(Func func) {
    this->func =
        std::make_shared<LambdaCb<Func, _Res, _ArgTypes...>>(std::move(func));
    return *this;
  }

  _Res impl(_ArgTypes... args) {
    if (func) {
      if constexpr (std::is_void_v<_Res>)
        func->impl(std::forward<_ArgTypes>(args)...);
      else
        return func->impl(std::forward<_ArgTypes>(args)...);
    } else {
      if constexpr (!std::is_void_v<_Res>)
        return _Res();
    }
  }

  _Res operator()(_ArgTypes... args) {
    if constexpr (std::is_void_v<_Res>)
      impl(std::forward<_ArgTypes>(args)...);
    else
      return impl(std::forward<_ArgTypes>(args)...);
  }

private:
  std::shared_ptr<ICallback<_Res, _ArgTypes...>> func;
};

template <typename T, typename F>
auto mapV(std::vector<T> const &v, F const &f) {
  using R = decltype(std::declval<F &>()(std::declval<T const &>()));
  std::vector<R> rs;
  for (auto const &x : v)
    rs.emplace_back(f(x));
  return rs;
}

std::vector<char const *> vkMapNames(std::vector<std::string> const &names);

#define VK_CHECK(expr)                                                         \
  ([&]() -> void {                                                             \
    VkResult __result = (expr);                                                \
    if (__result != VK_SUCCESS) {                                              \
      std::stringstream error_ss;                                              \
      error_ss << "Vk error [" << string_VkResult(__result) << "] ("           \
               << (#expr) << ")";                                              \
      throw std::runtime_error(error_ss.str());                                \
    }                                                                          \
  })()

template <typename Iter, typename Key>
auto find_max(Iter begin, Iter end, Key key) {
  Iter best = begin;
  auto bestScore = key(*best);
  for (Iter cur = begin; cur != end; ++cur) {
    auto curScore = key(*cur);
    if (bestScore < curScore) {
      best = cur;
      bestScore = curScore;
    }
  }
  return best;
}

std::string readFile(std::istream &is);

template <typename T>
std::shared_ptr<T> stack_ptr(T &value) {
  return std::shared_ptr<T>(&value, [](void *) -> void {});
}

template <typename T, typename... Refs>
class Handle {
public:
  Handle() = default;

  using Deleter = std::function<void(T, Refs &...)>;

  Handle(T value, Deleter deleter, std::shared_ptr<Refs>... refs)
      : value{std::move(value)}, deleter{std::move(deleter)},
        refs{std::make_tuple(std::move(refs)...)} {}

  Handle(Handle const &) = delete;
  Handle &operator=(Handle const &) = delete;

  Handle(Handle &&other) {
    *this = std::move(other);
  }

  Handle &operator=(Handle &&other) {
    if (this != &other) {
      destroy();
      value = std::move(other.value);
      other.value = {};
      refs = std::move(other.refs);
      deleter = std::move(other.deleter);
    }
    return *this;
  }

  ~Handle() {
    destroy();
  }

  operator T &() {
    return value;
  }

  operator T const &() const {
    return value;
  }

protected:
  T value = {};
  std::tuple<std::shared_ptr<Refs>...> refs;
  Deleter deleter = [](T value, Refs &...refs) -> void {};

private:
  void destroy() {
    _destroy(
        std::make_index_sequence<std::tuple_size_v<std::tuple<Refs...>>>{});
  }

  template <std::size_t... I>
  void _destroy(std::index_sequence<I...>) {
    if (value != T())
      deleter(value, *std::get<I>(refs)...);
    value = {};
  }
};