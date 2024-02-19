#include <eldr/core/logger.hpp>
#include <eldr/gui/gui.hpp>
#include <eldr/render/vulkan-wrapper.hpp>

#include <imgui.h>
#include <stdexcept>

namespace eldr {
static void glfwErrorCallback(int error, const char* description)
{
  spdlog::error("GLFW Error %d: %s", error, description);
}

EldrGUI::EldrGUI(int width, int height, std::string name)
  : width_{ width }, height_{ height }, window_name_{ name }
{
  // Initialize GLFW
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) {
    Throw("[GLFW]: Failed to initialize");
  }

  // Vulkan pre-check
  if (!glfwVulkanSupported()) {
    Throw("[GLFW]: Vulkan not supported");
  }

  // Create GLFW Window with Vulkan context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ =
    glfwCreateWindow(width_, height_, window_name_.c_str(), nullptr, nullptr);

  std::vector<const char*> extensions;
  uint32_t                 extensions_count = 0;
  const char**             glfw_extensions =
    glfwGetRequiredInstanceExtensions(&extensions_count);
  for (size_t i = 0; i < extensions_count; i++)
    extensions.push_back(glfw_extensions[i]);

  vk_wrapper_ = std::make_unique<vk::VulkanWrapper>(window_, extensions);
};

EldrGUI::~EldrGUI()
{
  glfwDestroyWindow(window_);
  glfwTerminate();
}

void EldrGUI::display() { vk_wrapper_->drawFrame(); }

} // Namespace eldr
