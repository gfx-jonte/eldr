#pragma once

#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/device.hpp>

namespace eldr {
namespace vk {

class Fence {
public:
  Fence();
  Fence(const Device* device_);
  Fence(Fence&&);
  ~Fence();

  const VkFence& get() const { return fence_; }
  VkFence& get() { return fence_; }
  void reset();
  void wait();

private:
  const Device* device_;

  VkFence fence_;
};
} // namespace vk
} // namespace eldr
