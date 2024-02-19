#pragma once

#include <eldr/vulkan/vulkan.hpp>

#include <string>

namespace eldr {

class EldrGUI {
public:
  EldrGUI() = delete;
  EldrGUI(int width, int height, std::string name);
  ~EldrGUI();

  void display();

  inline bool        shouldClose() { return glfwWindowShouldClose(window_); }
  inline GLFWwindow* getGLFWwindow() { return window_; };

private:
  const int   width_;
  const int   height_;
  std::string window_name_;

  GLFWwindow* window_;

  std::unique_ptr<vk::VulkanWrapper> vk_wrapper_;
};
} // namespace eldr
