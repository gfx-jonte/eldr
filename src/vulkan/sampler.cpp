#include <eldr/core/logger.hpp>
#include <eldr/vulkan/sampler.hpp>

namespace eldr {
namespace vk {

Sampler::Sampler(const Device* device, uint32_t mip_levels) : device_(device)
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter    = VK_FILTER_LINEAR;
  sampler_info.minFilter    = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(device_->physical(), &props);
  sampler_info.anisotropyEnable        = VK_TRUE;
  sampler_info.maxAnisotropy           = props.limits.maxSamplerAnisotropy;
  sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable           = VK_FALSE;
  sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias              = 0.0f;
  sampler_info.minLod                  = 0.0f;
  sampler_info.maxLod                  = static_cast<float>(mip_levels);
  if (vkCreateSampler(device_->logical(), &sampler_info, nullptr, &sampler_) !=
      VK_SUCCESS) {
    ThrowVk("Failed to create texture sampler!");
  }
}

Sampler::~Sampler()
{
  if (sampler_ != VK_NULL_HANDLE)
    vkDestroySampler(device_->logical(), sampler_, nullptr);
}

} // namespace vk
} // namespace eldr
