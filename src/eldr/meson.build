src = ['main.cpp', 'app.cpp']
executable('eldr',
  sources : src,
  include_directories : [include, inc_cxxopts, inc_imgui],
  dependencies : [libglfw, libvulkan, libspdlog],
  link_with : [imgui, eldr_vulkan, eldr_render])
