#pragma once

#include <glm/fwd.hpp>

namespace eldr {

template <size_t size, typename T> using Color = glm::vec<size, T>;
template <size_t size, typename T> using Point = glm::vec<size, T>;

// TODO: experiment with glm SIMD types and compare performance
struct CoreAliases {
  using Vec2i = glm::vec<2, glm::int32_t>;
  using Vec3i = glm::vec<3, glm::int32_t>;
  using Vec4i = glm::vec<4, glm::int32_t>;

  using Vec2u = glm::vec<2, glm::uint32_t>;
  using Vec3u = glm::vec<3, glm::uint32_t>;
  using Vec4u = glm::vec<4, glm::uint32_t>;

  using Vec2f = glm::vec<2, glm::float32_t>;
  using Vec3f = glm::vec<3, glm::float32_t>;
  using Vec4f = glm::vec<4, glm::float32_t>;

  using Vec2d = glm::vec<2, glm::float64_t>;
  using Vec3d = glm::vec<3, glm::float64_t>;
  using Vec4d = glm::vec<4, glm::float64_t>;

  using Point2i = Point<2, glm::int32_t>;
  using Point3i = Point<3, glm::int32_t>;
  using Point4i = Point<4, glm::int32_t>;

  using Point2u = Point<2, glm::uint32_t>;
  using Point3u = Point<3, glm::uint32_t>;
  using Point4u = Point<4, glm::uint32_t>;

  using Point2f = Point<2, glm::float32_t>;
  using Point3f = Point<3, glm::float32_t>;
  using Point4f = Point<4, glm::float32_t>;

  using Point2d = Point<2, glm::float64_t>;
  using Point3d = Point<3, glm::float64_t>;
  using Point4d = Point<4, glm::float64_t>;

  using Mat2f = glm::mat<2, 2, glm::float32_t>;
  using Mat3f = glm::mat<3, 3, glm::float32_t>;
  using Mat4f = glm::mat<4, 4, glm::float32_t>;

  using Mat2d = glm::mat<2, 2, glm::float64_t>;
  using Mat3d = glm::mat<3, 3, glm::float64_t>;
  using Mat4d = glm::mat<4, 4, glm::float64_t>;

  using Quat4f = glm::qua<glm::float32_t>;
  using Quat4d = glm::qua<glm::float64_t>;

  using Color1f = Color<1, glm::float32_t>;
  using Color3f = Color<3, glm::float32_t>;
  using Color1d = Color<1, glm::float64_t>;
  using Color3d = Color<3, glm::float64_t>;
};

#define ELDR_IMPORT_CORE_TYPES_PREFIX(prefix)                          \
  using prefix##CoreAliases = eldr::CoreAliases;                               \
  using prefix##Vec2i       = typename prefix##CoreAliases::Vec2i;             \
  using prefix##Vec3i       = typename prefix##CoreAliases::Vec3i;             \
  using prefix##Vec4i       = typename prefix##CoreAliases::Vec4i;             \
  using prefix##Vec2u       = typename prefix##CoreAliases::Vec2u;             \
  using prefix##Vec3u       = typename prefix##CoreAliases::Vec3u;             \
  using prefix##Vec4u       = typename prefix##CoreAliases::Vec4u;             \
  using prefix##Vec2f       = typename prefix##CoreAliases::Vec2f;             \
  using prefix##Vec3f       = typename prefix##CoreAliases::Vec3f;             \
  using prefix##Vec4f       = typename prefix##CoreAliases::Vec4f;             \
  using prefix##Vec2d       = typename prefix##CoreAliases::Vec2d;             \
  using prefix##Vec3d       = typename prefix##CoreAliases::Vec3d;             \
  using prefix##Vec4d       = typename prefix##CoreAliases::Vec4d;             \
  using prefix##Point2i     = typename prefix##CoreAliases::Point2i;           \
  using prefix##Point3i     = typename prefix##CoreAliases::Point3i;           \
  using prefix##Point4i     = typename prefix##CoreAliases::Point4i;           \
  using prefix##Point2u     = typename prefix##CoreAliases::Point2u;           \
  using prefix##Point3u     = typename prefix##CoreAliases::Point3u;           \
  using prefix##Point4u     = typename prefix##CoreAliases::Point4u;           \
  using prefix##Point2f     = typename prefix##CoreAliases::Point2f;           \
  using prefix##Point3f     = typename prefix##CoreAliases::Point3f;           \
  using prefix##Point4f     = typename prefix##CoreAliases::Point4f;           \
  using prefix##Point2d     = typename prefix##CoreAliases::Point2d;           \
  using prefix##Point3d     = typename prefix##CoreAliases::Point3d;           \
  using prefix##Point4d     = typename prefix##CoreAliases::Point4d;           \
  using prefix##Mat2f       = typename prefix##CoreAliases::Mat2f;             \
  using prefix##Mat2d       = typename prefix##CoreAliases::Mat2d;             \
  using prefix##Mat3f       = typename prefix##CoreAliases::Mat3f;             \
  using prefix##Mat3d       = typename prefix##CoreAliases::Mat3d;             \
  using prefix##Mat4f       = typename prefix##CoreAliases::Mat4f;             \
  using prefix##Mat4d       = typename prefix##CoreAliases::Mat4d;             \
  using prefix##Quat4f      = typename prefix##CoreAliases::Quat4f;            \
  using prefix##Quat4d      = typename prefix##CoreAliases::Quat4d;            \
  using prefix##Color1f     = typename prefix##CoreAliases::Color1f;           \
  using prefix##Color3f     = typename prefix##CoreAliases::Color3f;           \
  using prefix##Color1d     = typename prefix##CoreAliases::Color1d;           \
  using prefix##Color3d     = typename prefix##CoreAliases::Color3d;

#define ELDR_IMPORT_CORE_TYPES() ELDR_IMPORT_CORE_TYPES_PREFIX()

} // namespace eldr
