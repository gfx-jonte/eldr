#include <eldr/core/logger.hpp>
#include <eldr/vulkan/descriptorpool.hpp>

#include <vector>

namespace eldr {
namespace vk {

DescriptorPool::DescriptorPool(
  const Device* device, const std::vector<VkDescriptorPoolSize>& pool_sizes,
  uint32_t max_sets)
  : device_(device)
{
  /**
   * From Vulkan tutorial:
   *
   * Inadequate descriptor pools are a good example of a problem that the
   * validation layers will not catch: As of Vulkan 1.1,
   * vkAllocateDescriptorSets may fail with the error code
   * VK_ERROR_POOL_OUT_OF_MEMORY if the pool is not sufficiently large, but the
   * driver may also try to solve the problem internally. This means that
   * sometimes (depending on hardware, pool size and allocation size) the driver
   * will let us get away with an allocation that exceeds the limits of our
   * descriptor pool. Other times, vkAllocateDescriptorSets will fail and return
   * VK_ERROR_POOL_OUT_OF_MEMORY. This can be particularly frustrating if the
   * allocation succeeds on some machines, but fails on others.
   */

  VkDescriptorPoolCreateInfo pool_ci{};
  pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_ci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_ci.pPoolSizes    = pool_sizes.data();
  pool_ci.maxSets       = max_sets;

  if (vkCreateDescriptorPool(device_->logical(), &pool_ci, nullptr,
                             &descriptor_pool_) != VK_SUCCESS)
    ThrowVk("Failed to create descriptor pool!");
}
DescriptorPool::~DescriptorPool()
{
  if (descriptor_pool_ != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device_->logical(), descriptor_pool_, nullptr);
}
} // namespace vk
} // namespace eldr
