#pragma once
#include <glm/glm.hpp>

template <typename T>
struct vkalign {
  static constexpr std::size_t value = sizeof(T);
};

#define VK_ALIGN(t) alignas(vkalign<t>::value) t

template <typename T>
struct vkalign<glm::vec<3, T>> {
  static constexpr std::size_t value = 4 * sizeof(T);
};

template <std::size_t N, std::size_t M, typename T>
struct vkalign<glm::mat<N, M, T>> {
  static constexpr std::size_t value = sizeof(glm::vec4);
};