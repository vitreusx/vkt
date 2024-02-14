#include <vkx/stb_image.h>

StbImage::StbImage(std::filesystem::path const &filename, int mode) {
  int true_mode;
  this->mode = mode;
  data = stbi_load(filename.c_str(), &width, &height, &true_mode, mode);
}

StbImage::StbImage(StbImage &&other) {
  *this = std::move(other);
}

StbImage &StbImage::operator=(StbImage &&other) {
  destroy();
  width = other.width;
  height = other.height;
  data = other.data;
  other.data = nullptr;
  mode = other.mode;
  return *this;
}

StbImage::~StbImage() {
  destroy();
}

int StbImage::nbytes() const {
  return static_cast<int>(mode) * width * height;
}

void StbImage::destroy() {
  if (data != nullptr)
    stbi_image_free(data);
  data = nullptr;
}