src = [
  'buffer.cpp',
  'commandbuffer.cpp',
  'commandpool.cpp',
  'descriptorpool.cpp',
  'descriptorsetlayout.cpp',
  'device.cpp',
  'fence.cpp',
  'helpers.cpp',
  'image.cpp',
  'imageview.cpp',
  'instance.cpp',
  'pipeline.cpp',
  'renderpass.cpp',
  'sampler.cpp',
  'semaphore.cpp',
  'surface.cpp',
  'swapchain.cpp',
  'texture.cpp',
  'vulkan.cpp',
  'vertex.cpp'
  ]
eldr_vulkan = library(
  'eldr-vulkan',
  sources : src,
  include_directories : [include, inc_imgui],
  dependencies : [libglfw, libvulkan, libspdlog],
  link_with : [eldr_core])
  
