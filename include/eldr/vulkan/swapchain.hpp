#pragma once

#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/device.hpp>
#include <eldr/vulkan/image.hpp>
#include <eldr/vulkan/imageview.hpp>
#include <eldr/vulkan/renderpass.hpp>

namespace eldr {
namespace vk {
struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   present_modes;
};

class Swapchain {
public:
  Swapchain(const Device*, Surface&, GLFWwindow* const);
  ~Swapchain();

  const VkExtent2D&     extent() const { return extent_; }
  VkFormat              format() const { return image_format_; }
  const VkSwapchainKHR& get() const { return swapchain_; }
  VkSwapchainKHR&       get() { return swapchain_; }

  void recreate(Surface& surface, GLFWwindow* const window);

private:
  void createSwapchain(Surface&);
  void createFramebuffers();
  void createImageViews();
  void cleanup();

private:
  const Device*           device_;
  SwapchainSupportDetails support_details_;
  VkExtent2D              extent_;
  VkFormat                image_format_;
  VkSwapchainKHR          swapchain_;
  uint32_t                min_image_count_;
  uint32_t                image_count_;
  Image                   depth_image_;
  ImageView               depth_image_view_;

  std::vector<VkImage> images_; // needs to be VkImage unless array is converted
                                // to this type before call to
                                // vkGetSwapchainImagesKHR
  std::vector<ImageView> image_views_;

public:
  RenderPass                 render_pass_;
  std::vector<VkFramebuffer> framebuffers_;
};
} // namespace vk
} // namespace eldr
