#include <eldr/core/logger.hpp>
#include <eldr/vulkan/device.hpp>
#include <eldr/vulkan/helpers.hpp>

#include <set>

namespace eldr {
namespace vk {

/**
 * fwd helpers
 */
static void selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                                 VkPhysicalDevice&         physical_device,
                                 std::vector<const char*>& device_extensions);
static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                             std::vector<const char*> device_extensions);
// -----------------------------------------------------------------------------
// Device::Device() : physical_device_(VK_NULL_HANDLE),
// device_(VK_NULL_HANDLE){};

Device::Device(const Instance& instance, const Surface& surface)
{
  std::vector<const char*> device_extensions;
  device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
  device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  selectPhysicalDevice(instance.get(), surface.get(), physical_device_,
                       device_extensions);

  QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device_, surface.get());
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t>                   unique_queue_families = {
    queue_family_indices.graphics_family.value(),
    queue_family_indices.present_family.value()
  };

  float queue_priority = 1.0f;
  for (uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount       = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};
  device_features.samplerAnisotropy = VK_TRUE;
  device_features.sampleRateShading = VK_TRUE;

  // Logical device creation
  VkDeviceCreateInfo create_info = {};
  create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount =
    static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.enabledExtensionCount =
    static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();
  create_info.pEnabledFeatures        = &device_features;
  VkResult err =
    vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
  if (err != VK_SUCCESS)
    ThrowVk("Failed to create device!");

  vkGetDeviceQueue(device_, queue_family_indices.present_family.value(),
                   0, &p_queue_);
  vkGetDeviceQueue(device_, queue_family_indices.graphics_family.value(),
                   0, &g_queue_);
}

Device::~Device()
{
  if (device_ != VK_NULL_HANDLE)
    vkDestroyDevice(device_, nullptr);
}

/**
 * Helper definitions
 */
void selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                          VkPhysicalDevice&         physical_device,
                          std::vector<const char*>& device_extensions)
{
  uint32_t gpu_count;
  VkResult err = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
  if (err != VK_SUCCESS)
    ThrowVk("Failed to enumerate physical devices!");

  if (gpu_count == 0)
    ThrowVk("No compatible device found");

  std::vector<VkPhysicalDevice> gpus;
  gpus.resize(gpu_count);
  err = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
  if (err != VK_SUCCESS)
    ThrowVk("Failed to enumerate physical devices!");

  // Find discrete GPU if available
  // It is possible to add additional criteria for what is to be considered
  // a suitable GPU. The Vulkan programming tutorial suggests implementing a
  // scoring system where each usable property of a device would add to its
  // overall score, and when the properties of all devices have been
  // evaluated, the highest scoring device is considered the most suitable
  // one. For now I don't really think it matters. Just use a discrete GPU if
  // it's available.
  for (VkPhysicalDevice& device : gpus) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        isDeviceSuitable(device, surface, device_extensions)) {
      physical_device = device;
      return;
    }
  }
  // Return integrated GPU if no discrete one is present
  physical_device = gpus[0];
}

static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                             std::vector<const char*> device_extensions)
{
  // Enumerate physical device extension
  uint32_t                           props_count;
  std::vector<VkExtensionProperties> properties;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &props_count, nullptr);
  properties.resize(props_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &props_count,
                                       properties.data());

  // Check extensions
  std::set<std::string> required_extensions(device_extensions.begin(),
                                            device_extensions.end());
  for (const auto& extension : properties) {
    required_extensions.erase(extension.extensionName);
  }
  // Queue families
  QueueFamilyIndices indices = findQueueFamilies(device, surface);
  // Swap chain
  SwapchainSupportDetails swapchain_support =
    swapchainSupportDetails(device, surface);

  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(device, &supported_features);

  return required_extensions.empty() && indices.isComplete() &&
         !swapchain_support.formats.empty() &&
         !swapchain_support.present_modes.empty() &&
         supported_features.samplerAnisotropy;
}
} // namespace vk
} // namespace eldr
