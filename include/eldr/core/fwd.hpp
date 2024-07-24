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

#define ELDR_IMPORT_CORE_TYPES()                                               \
  using CoreAliases = eldr::CoreAliases;                                       \
  using Vec2i       = typename CoreAliases::Vec2i;                             \
  using Vec3i       = typename CoreAliases::Vec3i;                             \
  using Vec4i       = typename CoreAliases::Vec4i;                             \
  using Vec2u       = typename CoreAliases::Vec2u;                             \
  using Vec3u       = typename CoreAliases::Vec3u;                             \
  using Vec4u       = typename CoreAliases::Vec4u;                             \
  using Vec2f       = typename CoreAliases::Vec2f;                             \
  using Vec3f       = typename CoreAliases::Vec3f;                             \
  using Vec4f       = typename CoreAliases::Vec4f;                             \
  using Vec2d       = typename CoreAliases::Vec2d;                             \
  using Vec3d       = typename CoreAliases::Vec3d;                             \
  using Vec4d       = typename CoreAliases::Vec4d;                             \
  using Point2i     = typename CoreAliases::Point2i;                           \
  using Point3i     = typename CoreAliases::Point3i;                           \
  using Point4i     = typename CoreAliases::Point4i;                           \
  using Point2u     = typename CoreAliases::Point2u;                           \
  using Point3u     = typename CoreAliases::Point3u;                           \
  using Point4u     = typename CoreAliases::Point4u;                           \
  using Point2f     = typename CoreAliases::Point2f;                           \
  using Point3f     = typename CoreAliases::Point3f;                           \
  using Point4f     = typename CoreAliases::Point4f;                           \
  using Point2d     = typename CoreAliases::Point2d;                           \
  using Point3d     = typename CoreAliases::Point3d;                           \
  using Point4d     = typename CoreAliases::Point4d;                           \
  using Mat2f       = typename CoreAliases::Mat2f;                             \
  using Mat2d       = typename CoreAliases::Mat2d;                             \
  using Mat3f       = typename CoreAliases::Mat3f;                             \
  using Mat3d       = typename CoreAliases::Mat3d;                             \
  using Mat4f       = typename CoreAliases::Mat4f;                             \
  using Mat4d       = typename CoreAliases::Mat4d;                             \
  using Quat4f      = typename CoreAliases::Quat4f;                            \
  using Quat4d      = typename CoreAliases::Quat4d;                            \
  using Color1f     = typename CoreAliases::Color1f;                           \
  using Color3f     = typename CoreAliases::Color3f;                           \
  using Color1d     = typename CoreAliases::Color1d;                           \
  using Color3d     = typename CoreAliases::Color3d;

} // namespace eldr
