#include <vkx/obj_model.h>
#include <spdlog/spdlog.h>

#define CHECK(expr)                                                            \
  ([&]() -> void {                                                             \
    auto __result = (expr);                                                    \
    if (!__result)                                                             \
      throw std::runtime_error("Error: " #expr);                               \
  })()

ObjModel::ObjModel(std::filesystem::path const &path) {
  this->path = path;
  root = path.parent_path();

  std::string warn, err;
  CHECK(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                         path.c_str(), root.c_str()));

  if (!warn.empty())
    spdlog::warn(warn);

  if (!err.empty())
    throw std::runtime_error(err);
}