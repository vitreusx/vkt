#pragma once
#include <filesystem>
#include <optional>
#include <stb_image.h>

namespace vkext {
class StbImage {
public:
  StbImage(std::filesystem::path const &filename, int mode = STBI_default);
  StbImage(const stbi_uc *buffer, int len, int mode = STBI_default);

  StbImage(StbImage const &) = delete;
  StbImage &operator=(StbImage const &) = delete;

  StbImage(StbImage &&other);
  StbImage &operator=(StbImage &&other);

  ~StbImage();

  int width, height;
  stbi_uc *data;
  int mode;

  int nbytes() const;

private:
  void destroy();
};

}