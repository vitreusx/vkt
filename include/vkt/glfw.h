#pragma once
#include <memory>
#include <vkt/utils.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class GLFWLib {
public:
  static std::shared_ptr<GLFWLib> getInstance(Callback<GLFWerrorfun> cb);

  GLFWLib(GLFWLib const &) = delete;

private:
  GLFWLib();
  ~GLFWLib();

  static Callback<GLFWerrorfun> onError;

  static void _onError(int errorCode, char const *description);
};

class Window {
public:
  Window() = default;
  Window(std::shared_ptr<GLFWLib> glfw, int width, int height,
         char const *title);

  Window(Window const &) = delete;
  Window &operator=(Window const &) = delete;

  Window(Window &&other);
  Window &operator=(Window &&other);

  ~Window();
  void destroy();

  operator GLFWwindow *();

  std::pair<int, int> getWindowSize();

public:
  typedef void (*OnKey)(int key, int scanCode, int action, int mods);
  Callback<OnKey> onKey;

  typedef void (*OnFramebufferSize)(int width, int height);
  Callback<OnFramebufferSize> onFramebufferSize;

  typedef void (*OnCursorPos)(double xpos, double ypos);
  Callback<OnCursorPos> onCursorPos;

  typedef void (*OnScroll)(double xoffset, double yoffset);
  Callback<OnScroll> onScroll;

private:
  std::shared_ptr<GLFWLib> glfw;
  GLFWwindow *handle = nullptr;

  void resetCallbacks();

  static void _onKey(GLFWwindow *window, int key, int scanCode, int action,
                     int mods);
  static void _onFramebufferSize(GLFWwindow *window, int width, int height);
  static void _onCursorPos(GLFWwindow *window, double xpos, double ypos);
  static void _onScroll(GLFWwindow *window, double xoffset, double yoffset);
};