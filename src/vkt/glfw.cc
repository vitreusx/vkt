#include <vkt/glfw.h>

std::shared_ptr<GLFWLib> GLFWLib::getInstance(Callback<GLFWerrorfun> cb) {
  static std::shared_ptr<GLFWLib> instance;
  if (!instance) {
    struct _GLFWLib : public GLFWLib {};
    instance = std::make_shared<_GLFWLib>();
    onError = std::move(cb);
  }
  return instance;
}

GLFWLib::GLFWLib() {
  glfwSetErrorCallback(_onError);
  glfwInit();
}

GLFWLib::~GLFWLib() {
  glfwTerminate();
}

void GLFWLib::_onError(int errorCode, char const *description) {
  onError(errorCode, description);
}

Callback<GLFWerrorfun> GLFWLib::onError;

Window::Window(std::shared_ptr<GLFWLib> glfw, int width, int height,
               char const *title) {
  this->glfw = std::move(glfw);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  this->handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
  resetCallbacks();
}

Window::Window(Window &&other) {
  *this = std::move(other);
}

Window &Window::operator=(Window &&other) {
  destroy();
  onKey = std::move(other.onKey);
  glfw = std::move(other.glfw);
  handle = other.handle;
  other.handle = nullptr;
  resetCallbacks();
  return *this;
}

Window::~Window() {
  destroy();
}

void Window::destroy() {
  if (handle)
    glfwDestroyWindow(handle);
  handle = nullptr;
}

Window::operator GLFWwindow *() {
  return handle;
}

void Window::resetCallbacks() {
  glfwSetWindowUserPointer(this->handle, this);
  glfwSetKeyCallback(this->handle, _onKey);
  glfwSetFramebufferSizeCallback(this->handle, _onFramebufferSize);
}

void Window::_onKey(GLFWwindow *window, int key, int scanCode, int action,
                    int mods) {
  Window *self = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  self->onKey(key, scanCode, action, mods);
}

void Window::_onFramebufferSize(GLFWwindow *window, int width, int height) {
  Window *self = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  self->onFramebufferSize(width, height);
}