#pragma once
#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/swapchain.hpp>
#include <eldr/vulkan/device.hpp>

namespace eldr {
namespace vk {

SwapChainSupportDetails swapChainSupportDetails(VkPhysicalDevice, VkSurfaceKHR);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice, VkSurfaceKHR);

uint32_t findMemoryType(VkPhysicalDevice, uint32_t type_filter,
                        VkMemoryPropertyFlags);
} // namespace vk
} // namespace eldr
