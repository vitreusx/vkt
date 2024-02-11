#include <vkt/pipeline.h>

Pipeline::Pipeline(std::shared_ptr<Device> device, VkPipeline &&pipeline)
    : device{std::move(device)}, pipeline{std::move(pipeline)} {}

Pipeline::Pipeline(Pipeline &&other) {
  *this = std::move(other);
}

Pipeline &Pipeline::operator=(Pipeline &&other) {
  destroy();
  device = std::move(other.device);
  pipeline = other.pipeline;
  other.pipeline = VK_NULL_HANDLE;
  return *this;
}

Pipeline::~Pipeline() {
  destroy();
}

void Pipeline::destroy() {
  if (pipeline != VK_NULL_HANDLE)
    device->vkDestroyPipeline(*device, pipeline, VK_NULL_HANDLE);
  pipeline = VK_NULL_HANDLE;
}

Pipeline::operator VkPipeline() {
  return pipeline;
}