#pragma once
#include <vkt/device.h>

struct ShaderModuleCreateInfo {
  std::string code;
};

class ShaderModule {
public:
  ShaderModule(std::shared_ptr<Device> device,
               ShaderModuleCreateInfo const &createInfo);

  ShaderModule(ShaderModule const &) = delete;
  ShaderModule &operator=(ShaderModule const &) = delete;

  ShaderModule(ShaderModule &&other);
  ShaderModule &operator=(ShaderModule &&other);

  ~ShaderModule();

  operator VkShaderModule();

private:
  std::shared_ptr<Device> device = {};
  VkShaderModule shaderModule = VK_NULL_HANDLE;

  void destroy();
};