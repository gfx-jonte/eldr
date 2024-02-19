#include <eldr/eldr.hpp>

namespace eldr {


void EldrApp::run()
{
  while (!gui_.shouldClose()) {
    glfwPollEvents();
    scene_.update();
    gui_.display();
  }
}

} // Namespace eldr
