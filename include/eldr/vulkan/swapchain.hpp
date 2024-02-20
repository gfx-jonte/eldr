#pragma once

#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/device.hpp>
#include <eldr/vulkan/image.hpp>
#include <eldr/vulkan/imageview.hpp>
#include <eldr/vulkan/renderpass.hpp>

namespace eldr {
namespace vk {
struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   present_modes;
};

class Swapchain {
public:
  Swapchain(const Device*, Surface&, GLFWwindow*);
  ~Swapchain();

  const VkExtent2D&     extent() const { return extent_; }
  VkFormat              format() const { return image_format_; }
  const VkSwapchainKHR& get() const { return swapchain_; }
  VkSwapchainKHR&       get() { return swapchain_; }
  const VkFramebuffer&  framebuffer(uint32_t index) const
  {
    return framebuffers_[index];
  }

  void createFramebuffers(RenderPass&);

private:
  void clean();
  // void recreate();

private:
  // Create info
  const Device* device_;

  VkSwapchainKHR swapchain_;
  VkExtent2D     extent_;
  VkFormat       image_format_;
  uint32_t       min_image_count_;
  uint32_t       image_count_;

  std::vector<VkImage>       images_;
  std::vector<ImageView>     image_views_;
  std::vector<VkFramebuffer> framebuffers_;
};
} // namespace vk
} // namespace eldr
