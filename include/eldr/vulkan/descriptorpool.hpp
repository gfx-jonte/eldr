#pragma once

#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/device.hpp>

namespace eldr {
namespace vk {

// TODO: this can perhaps be merged with DescriptorSetLayout somehow
class DescriptorPool {
public:
  DescriptorPool(const Device*, const std::vector<VkDescriptorPoolSize>&,
                 VkDescriptorPoolCreateFlags flags, uint32_t max_sets);
  ~DescriptorPool();

  const VkDescriptorPool& get() const { return descriptor_pool_; }
  VkDescriptorPool&       get() { return descriptor_pool_; }

private:
  const Device* device_;

  VkDescriptorPool descriptor_pool_;
};
} // namespace vk
} // namespace eldr
