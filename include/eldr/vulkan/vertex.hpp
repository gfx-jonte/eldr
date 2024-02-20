#pragma once
#include <eldr/core/fwd.hpp>
#include <eldr/vulkan/common.hpp>

#include <array>

namespace eldr {
namespace vk {

struct Vertex {
  ELDR_IMPORT_CORE_TYPES();
  Vec2f   pos;
  Color3f color;
  Vec2f   tex_coord;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions();
};
} // namespace vk
} // namespace eldr
