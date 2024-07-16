#pragma once

#include <eldr/core/fwd.hpp>

namespace eldr {

enum class ShapeType { Mesh, Disk, Rectangle, Sphere, Other };

class Shape {
public:
#ifdef ELDR_ENABLE_EMBREE
  virtual RTCGeometry embreeGeometry(RTCDevice device);
#endif

  friend class Scene;

protected:
  inline Shape(){};
  virtual ~Shape() = default;

protected:
  // BSDF bsdf_;
  //  Emitter emitter_;
  //  Sensor sensor_;
  //  Medium interior_medium_;
  //  Medium exterior_medium_;
  //std::string id_;
  ShapeType   shape_type_ = ShapeType::Other;
};
} // namespace eldr
