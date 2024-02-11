#pragma once
#include <vkt/device.h>

class Pipeline {
public:
  Pipeline() = default;
  Pipeline(std::shared_ptr<Device> device, VkPipeline &&pipeline);

  Pipeline(Pipeline const &) = delete;
  Pipeline &operator=(Pipeline const &) = delete;

  Pipeline(Pipeline &&other);
  Pipeline &operator=(Pipeline &&other);

  ~Pipeline();
  void destroy();

  operator VkPipeline();

protected:
  std::shared_ptr<Device> device = {};
  VkPipeline pipeline = VK_NULL_HANDLE;
};