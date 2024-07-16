#pragma once
#include <eldr/core/fwd.hpp>
#include <eldr/vulkan/common.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

namespace eldr {
namespace vk {

struct Vertex {
  ELDR_IMPORT_CORE_TYPES();
  Vec3f   position;
  Color3f color;
  Vec2f   tex_coord;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions();

  bool operator==(const Vertex& other) const
  {
    return position == other.position && color == other.color &&
           tex_coord == other.tex_coord;
  }
};
} // namespace vk
} // namespace eldr
namespace std {
template <> struct hash<eldr::vk::Vertex> {
  ELDR_IMPORT_CORE_TYPES();
  size_t operator()(eldr::vk::Vertex const& vertex) const;
};
} // namespace std
