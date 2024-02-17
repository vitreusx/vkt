#include <vkt/pipeline.h>

Pipeline::Pipeline(std::shared_ptr<Device> device, VkPipeline &&pipeline) {
  this->device = device;

  this->pipeline = Handle<VkPipeline, Device>(
      pipeline,
      [](VkPipeline pipeline, Device &device) -> void {
        device.vkDestroyPipeline(device, pipeline, nullptr);
      },
      device);
}

Pipeline::operator VkPipeline() {
  return pipeline;
}