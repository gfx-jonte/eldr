#pragma once
#include <eldr/core/fwd.hpp>
#include <eldr/render/shape.hpp>
#include <eldr/core/math.hpp>

#include <vector>
#include <string>

namespace eldr {
class Mesh : public Shape {
  ELDR_IMPORT_CORE_TYPES();

public:
  Mesh(const std::vector<Vec3f>& positions, const std::vector<Vec3f>& normals,
       const std::vector<Vec2f>& texcoords);

  const std::vector<Vec3f>& vertexPositions() { return vertex_positions_; }

  const std::vector<Vec3f>& vertexNormals() { return vertex_normals_; }

  const std::vector<Vec2f>& vertexTexCoords() { return vertex_texcoords_; }

protected:
  std::string name_;
  // uint32_t    vertex_count_ = 0;
  // uint32_t    face_count_   = 0;

  std::vector<Vec3f> vertex_positions_;
  std::vector<Vec3f> vertex_normals_;
  std::vector<Vec2f> vertex_texcoords_;
};
} // namespace eldr
