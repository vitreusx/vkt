#pragma once
#include <utility>
#include <memory>
#include <type_traits>
#include <istream>
#include <vector>
#include <fstream>

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
    if (__result != VK_SUCCESS)                                                \
      throw std::runtime_error("Vk error: " #expr);                            \
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

std::string readFile(std::ifstream &is);