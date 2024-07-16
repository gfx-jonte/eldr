#include <eldr/core/logger.hpp>
#include <eldr/vulkan/commandbuffer.hpp>

namespace eldr {
namespace vk {
CommandBuffer::CommandBuffer()
  : device_(nullptr), command_pool_(nullptr), command_buffer_(VK_NULL_HANDLE)
{
}

CommandBuffer::CommandBuffer(const Device*      device,
                             CommandPool* const command_pool)
  : device_(device), command_pool_(command_pool)
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_->get();
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  CheckVkResult(vkAllocateCommandBuffers(device_->logical(), &alloc_info,
                                         &command_buffer_));
}
CommandBuffer::~CommandBuffer()
{
  if (command_buffer_ != VK_NULL_HANDLE)
    vkFreeCommandBuffers(device_->logical(), command_pool_->get(), 1,
                         &command_buffer_);
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other)
{
  if (command_buffer_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(device_->logical(), command_pool_->get(), 1,
                         &command_buffer_);
  }
  device_         = other.device_;
  command_pool_   = other.command_pool_;
  command_buffer_ = other.command_buffer_;

  other.device_         = nullptr;
  other.command_pool_   = nullptr;
  other.command_buffer_ = VK_NULL_HANDLE;

  return *this;
}

void CommandBuffer::begin(VkCommandBufferUsageFlags usage)
{
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags            = usage;
  begin_info.pInheritanceInfo = nullptr;
  CheckVkResult(vkBeginCommandBuffer(command_buffer_, &begin_info));
}

void CommandBuffer::beginSingleCommand()
{
  begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void CommandBuffer::beginRenderPass(VkRenderPass  render_pass,
                                    VkFramebuffer framebuffer,
                                    VkExtent2D    extent,
                                    uint32_t      clear_value_count,
                                    VkClearValue* clear_values)
{
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = render_pass;
  render_pass_info.framebuffer       = framebuffer;
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = extent;
  render_pass_info.clearValueCount   = clear_value_count;
  render_pass_info.pClearValues      = clear_values;

  vkCmdBeginRenderPass(command_buffer_, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::endRenderPass() { vkCmdEndRenderPass(command_buffer_); }

void CommandBuffer::end()
{
  CheckVkResult(vkEndCommandBuffer(command_buffer_));
}

// TODO: may want to optimize this function if used often (use fences instead of
// vkQueueWaitIdle)
void CommandBuffer::submit()
{
  vkEndCommandBuffer(command_buffer_);

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &command_buffer_;

  VkResult res;
  res =
    vkQueueSubmit(device_->graphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
  CheckVkResult(res);
  res = vkQueueWaitIdle(device_->graphicsQueue());
  CheckVkResult(res);
}

void CommandBuffer::reset()
{
  CheckVkResult(vkResetCommandBuffer(command_buffer_, 0));
}

} // namespace vk
} // namespace eldr
