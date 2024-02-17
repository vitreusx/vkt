#pragma once
#include <vkt/device.h>

struct ShaderModuleCreateInfo {
  std::string code;
};

class ShaderModule {
public:
  ShaderModule() = default;
  ShaderModule(std::shared_ptr<Device> device,
               ShaderModuleCreateInfo const &createInfo);

  operator VkShaderModule();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkShaderModule, Device> shaderModule;
};