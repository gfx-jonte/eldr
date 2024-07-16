#include <eldr/render/mesh.hpp>

namespace eldr {

Mesh::Mesh(const std::vector<Vec3f>& positions,
           const std::vector<Vec3f>& normals,
           const std::vector<Vec2f>& texcoords)
  : vertex_positions_(std::move(positions)),
    vertex_normals_(std::move(normals)), vertex_texcoords_(std::move(texcoords))
{
}

} // namespace eldr
