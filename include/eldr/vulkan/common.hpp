#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#  define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vector>

namespace eldr {
namespace vk {
constexpr uint8_t max_frames_in_flight = 2;

} // namespace vk
} // namespace eldr
