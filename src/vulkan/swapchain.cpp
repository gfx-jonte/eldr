#include <eldr/core/logger.hpp>
#include <eldr/vulkan/helpers.hpp>
#include <eldr/vulkan/swapchain.hpp>

namespace eldr {
namespace vk {
// fwd -------------------------------------------------------------------------
static VkSurfaceFormatKHR
selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
static VkPresentModeKHR
                  selectSwapPresentMode(const std::vector<VkPresentModeKHR>&);
static VkExtent2D selectSwapExtent(GLFWwindow*,
                                   const VkSurfaceCapabilitiesKHR&);
VkSampleCountFlagBits getMsaaSampleCount(const Device&);
// -----------------------------------------------------------------------------

Swapchain::Swapchain(const Device* device, Surface& surface,
                     GLFWwindow* const window)
  : device_(device), support_details_(swapchainSupportDetails(
                       device_->physical(), surface.get())),
    extent_(selectSwapExtent(window, support_details_.capabilities)),
    image_format_(selectSwapSurfaceFormat(support_details_.formats).format),
    msaa_samples_(getMsaaSampleCount(*device_)),
    color_image_(device_, { { extent_.width, extent_.height },
                            image_format_,
                            VK_IMAGE_TILING_OPTIMAL,
                            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            msaa_samples_ }),
    color_image_view_(device_, color_image_.get(), color_image_.format(), 1,
                      VK_IMAGE_ASPECT_COLOR_BIT),
    depth_image_(device_, { { extent_.width, extent_.height },
                            findDepthFormat(device_->physical()),
                            VK_IMAGE_TILING_OPTIMAL,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            msaa_samples_ }),
    depth_image_view_(device_, depth_image_.get(), depth_image_.format(), 1,
                      VK_IMAGE_ASPECT_DEPTH_BIT),
    images_(), image_views_(),
    render_pass_(device_, image_format_, msaa_samples_), framebuffers_()
{
  createSwapchain(surface);
  createImageViews();
  createFramebuffers();
}

Swapchain::~Swapchain() { cleanup(); }

void Swapchain::createSwapchain(Surface& surface)
{
  surface.setFormat(selectSwapSurfaceFormat(support_details_.formats));
  surface.setPresentMode(selectSwapPresentMode(support_details_.present_modes));

  min_image_count_ = support_details_.capabilities.minImageCount + 1;
  // If there is an upper limit, make sure we don't exceed it
  if (support_details_.capabilities.maxImageCount > 0) {
    min_image_count_ =
      std::min(min_image_count_, support_details_.capabilities.maxImageCount);
  }
  VkSwapchainCreateInfoKHR swap_ci{};
  swap_ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_ci.surface          = surface.get();
  swap_ci.minImageCount    = min_image_count_;
  swap_ci.imageFormat      = surface.format().format;
  swap_ci.imageColorSpace  = surface.format().colorSpace;
  swap_ci.imageExtent      = extent_;
  swap_ci.imageArrayLayers = 1;
  swap_ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swap_ci.preTransform     = support_details_.capabilities.currentTransform;
  swap_ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_ci.presentMode      = surface.presentMode();
  swap_ci.clipped      = VK_TRUE; // don't care about color of obscured pixels
  swap_ci.oldSwapchain = VK_NULL_HANDLE;

  QueueFamilyIndices indices =
    findQueueFamilies(device_->physical(), surface.get());
  uint32_t queueFamilyIndices[] = { indices.graphics_family.value(),
                                    indices.present_family.value() };

  if (indices.graphics_family != indices.present_family) {
    swap_ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    swap_ci.queueFamilyIndexCount = 2;
    swap_ci.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else {
    swap_ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swap_ci.queueFamilyIndexCount = 0;       // Optional
    swap_ci.pQueueFamilyIndices   = nullptr; // Optional
  }

  if (vkCreateSwapchainKHR(device_->logical(), &swap_ci, nullptr,
                           &swapchain_) != VK_SUCCESS) {
    ThrowVk("Failed to create swapchain");
  }
  vkGetSwapchainImagesKHR(device_->logical(), swapchain_, &image_count_,
                          nullptr);
  images_.resize(image_count_);
  vkGetSwapchainImagesKHR(device_->logical(), swapchain_, &image_count_,
                          images_.data());
}

void Swapchain::createImageViews()
{
  // Create image views
  for (size_t i = 0; i < images_.size(); i++) {
    image_views_.emplace_back(ImageView(device_, images_[i], image_format_, 1));
  }
}

void Swapchain::cleanup()
{
  if (swapchain_ != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(device_->logical(), swapchain_, nullptr);

  image_views_.clear();

  for (size_t i = 0; i < framebuffers_.size(); ++i)
    if (framebuffers_[i] != VK_NULL_HANDLE)
      vkDestroyFramebuffer(device_->logical(), framebuffers_[i], nullptr);
}

void Swapchain::createFramebuffers()
{
  framebuffers_.resize(image_views_.size());
  for (size_t i = 0; i < image_views_.size(); ++i) {
    std::array<VkImageView, 3> attachments = { color_image_view_.get(),
                                               depth_image_view_.get(),
                                               image_views_[i].get() };
    VkFramebufferCreateInfo    framebuffer_ci{};
    framebuffer_ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.renderPass      = render_pass_.get();
    framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_ci.pAttachments    = attachments.data();
    framebuffer_ci.width           = extent_.width;
    framebuffer_ci.height          = extent_.height;
    framebuffer_ci.layers          = 1;

    if (vkCreateFramebuffer(device_->logical(), &framebuffer_ci, nullptr,
                            &framebuffers_[i]) != VK_SUCCESS)
      ThrowVk("Failed to create framebuffer!");
  }
}

void Swapchain::recreate(Surface& surface, GLFWwindow* const window)
{
  int width  = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device_->logical());

  cleanup();

  support_details_ =
    swapchainSupportDetails(device_->physical(), surface.get());
  extent_ = selectSwapExtent(window, support_details_.capabilities);

  color_image_ = Image(device_, { { extent_.width, extent_.height },
                                  image_format_,
                                  VK_IMAGE_TILING_OPTIMAL,
                                  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  msaa_samples_ });
  color_image_view_ =
    ImageView(device_, color_image_.get(), color_image_.format(), 1,
              VK_IMAGE_ASPECT_COLOR_BIT);

  depth_image_ = Image(device_, { { extent_.width, extent_.height },
                                  findDepthFormat(device_->physical()),
                                  VK_IMAGE_TILING_OPTIMAL,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  msaa_samples_ });
  depth_image_view_ =
    ImageView(device_, depth_image_.get(), depth_image_.format(), 1,
              VK_IMAGE_ASPECT_DEPTH_BIT);

  createSwapchain(surface);
  createImageViews();
  createFramebuffers();
}
VkResult Swapchain::acquireNextImage(uint32_t& index, Semaphore& semaphore) {
  VkResult result = vkAcquireNextImageKHR(
    device_->logical(), swapchain_, UINT64_MAX,
    semaphore.get(), VK_NULL_HANDLE, &index);
  return result;
}

// -----------------------------------------------------------------------------
static VkSurfaceFormatKHR selectSwapSurfaceFormat(
  const std::vector<VkSurfaceFormatKHR>& available_formats)
{
  for (const auto& a_format : available_formats) {
    if (a_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        a_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return a_format;
    }
  }
  return available_formats[0];
}

static VkPresentModeKHR selectSwapPresentMode(
  const std::vector<VkPresentModeKHR>& available_present_modes)
{
  for (const auto& a_present_mode : available_present_modes) {
    if (a_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      // prefer this if it exists (triple buffering)
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to exist (vertical sync)
}

static VkExtent2D selectSwapExtent(GLFWwindow*                     window,
                                   const VkSurfaceCapabilitiesKHR& capabilities)
{
  // Match the resolution of the current window, unless the value is the max
  // of uint32_t
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = { static_cast<uint32_t>(width),
                          static_cast<uint32_t>(height) };
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height =
      std::clamp(extent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);
    return extent;
  }
}
VkSampleCountFlagBits getMsaaSampleCount(const Device& device)
{
  VkPhysicalDeviceProperties physical_device_props{};
  vkGetPhysicalDeviceProperties(device.physical(), &physical_device_props);

  VkSampleCountFlags counts =
    physical_device_props.limits.framebufferColorSampleCounts &
    physical_device_props.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  return VK_SAMPLE_COUNT_1_BIT;
}
} // namespace vk
} // namespace eldr
