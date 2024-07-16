#pragma once

#include <eldr/vulkan/commandpool.hpp>
#include <eldr/vulkan/common.hpp>

namespace eldr {
namespace vk {

class CommandBuffer {
public:
  CommandBuffer();
  CommandBuffer(const Device* device_, CommandPool* const);
  ~CommandBuffer();

  CommandBuffer& operator=(CommandBuffer&&);

  VkCommandBuffer& get() { return command_buffer_; }
  void             begin(VkCommandBufferUsageFlags usage = 0);
  void             beginSingleCommand();
  void             submit();
  void             beginRenderPass(VkRenderPass, VkFramebuffer, VkExtent2D,
                                   uint32_t clear_value_count, VkClearValue*);
  void             endRenderPass();
  void             end();
  void             reset();

private:
  const Device* device_;
  CommandPool*  command_pool_;

  VkCommandBuffer command_buffer_;
};
} // namespace vk
} // namespace eldr
