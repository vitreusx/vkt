#include <vkt/shader_module.h>

ShaderModule::ShaderModule(std::shared_ptr<Device> device,
                           ShaderModuleCreateInfo const &createInfo) {
  this->device = device;

  VkShaderModuleCreateInfo vk_createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .codeSize = createInfo.code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(createInfo.code.c_str())};

  VkShaderModule shaderModule;
  VK_CHECK(device->vkCreateShaderModule(*device, &vk_createInfo, nullptr,
                                        &shaderModule));

  this->shaderModule = Handle<VkShaderModule, Device>(
      shaderModule,
      [](VkShaderModule shaderModule, Device &device) -> void {
        device.vkDestroyShaderModule(device, shaderModule, nullptr);
      },
      device);
}

ShaderModule::operator VkShaderModule() {
  return shaderModule;
}