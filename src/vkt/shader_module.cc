#include <vkt/shader_module.h>

ShaderModule::ShaderModule(std::shared_ptr<Device> device,
                           ShaderModuleCreateInfo const &createInfo) {
  this->device = device;

  VkShaderModuleCreateInfo vk_createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .codeSize = createInfo.code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(createInfo.code.c_str())};

  VK_CHECK(device->vkCreateShaderModule(*device, &vk_createInfo, VK_NULL_HANDLE,
                                        &shaderModule));
}

ShaderModule::ShaderModule(ShaderModule &&other) {
  *this = std::move(other);
}

ShaderModule &ShaderModule::operator=(ShaderModule &&other) {
  destroy();
  device = std::move(other.device);
  shaderModule = other.shaderModule;
  other.shaderModule = VK_NULL_HANDLE;
  return *this;
}

ShaderModule::~ShaderModule() {
  destroy();
}

void ShaderModule::destroy() {
  if (shaderModule != VK_NULL_HANDLE)
    device->vkDestroyShaderModule(*device, shaderModule, VK_NULL_HANDLE);
  shaderModule = VK_NULL_HANDLE;
}

ShaderModule::operator VkShaderModule() {
  return shaderModule;
}