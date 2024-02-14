#include <vkx/obj_model.h>
#include <stb_image.h>
#include <filesystem>
#include <optional>

enum class ImageMode : int {
  L = 1,
  RGB = 3,
  RGBA = 4
};

class StbImage {
public:
  StbImage(std::filesystem::path const &filename,
           std::optional<ImageMode> const &mode = std::nullopt) {
    int n = 0;
    if (mode.has_value())
      n = static_cast<int>(mode.value());

    int true_n;
    data = stbi_load(filename.c_str(), &width, &height, &true_n, n);

    this->mode = static_cast<ImageMode>(true_n);
  }

  StbImage(StbImage const &) = delete;
  StbImage &operator=(StbImage const &) = delete;

  StbImage(StbImage &&other) {
    *this = std::move(other);
  }

  StbImage &operator=(StbImage &&other) {
    destroy();
    width = other.width;
    height = other.height;
    data = other.data;
    other.data = nullptr;
    mode = other.mode;
    return *this;
  }

  ~StbImage() {
    destroy();
  }

  int width, height;
  stbi_uc *data;
  ImageMode mode;

  int nbytes() const {
    return static_cast<int>(mode) * width * height;
  }

private:
  void destroy() {
    if (data != nullptr)
      stbi_image_free(data);
    data = nullptr;
  }
};

int main() {
  auto model = ObjModel("assets/sibenik/sibenik.obj");
  return EXIT_SUCCESS;
}