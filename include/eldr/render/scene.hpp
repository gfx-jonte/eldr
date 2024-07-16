#pragma once
#include <eldr/core/fwd.hpp>
#include <eldr/render/shape.hpp>

#include <vector>
#include <string>

namespace eldr {
class Scene {
  ELDR_IMPORT_CORE_TYPES();
  struct SceneInfo {
    const std::string model_path;
    const std::string texture_path;
  };

public:
  Scene(const SceneInfo&);
  ~Scene();

  inline const std::vector<Shape*>& getShapes(){ return shapes_; }

private:
  std::vector<Shape*> loadGeometry(const SceneInfo&);

private:
  // std::vector<Emitter> emitters_;
  std::vector<Shape*> shapes_;
  // std::vector<Sensor> sensors_;
};
} // namespace eldr
