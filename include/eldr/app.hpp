#pragma once
#include <eldr/render/scene.hpp>

// -----------------------------------------------------------------------------
// fwd
// -----------------------------------------------------------------------------
struct GLFWwindow;
namespace eldr {
namespace vk {
class VulkanEngine;
}
// -----------------------------------------------------------------------------

class EldrApp {
  const std::string model_path_str   = "models/viking_room.obj";
  const std::string texture_path_str = "textures/viking_room.png";

public:
  EldrApp();
  ~EldrApp();

  void resize() const;
  void run();

private:
  void submitGeometry(const std::vector<Shape*>&);

public:
  static constexpr uint32_t width  = 1280;
  static constexpr uint32_t height = 720;

private:
  GLFWwindow*       window_;
  vk::VulkanEngine* vk_engine_;
  Scene             scene_;
};

} // Namespace eldr
