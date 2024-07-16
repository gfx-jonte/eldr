inc_cxxopts = include_directories('cxxopts/include')
inc_tinyobjloader = include_directories('tinyobjloader')

# ------------------------------------------------------------------------------
# Build Dear Imgui
# ------------------------------------------------------------------------------
inc_imgui = include_directories('imgui')
imgui = library(
  'imgui',
  sources : [
    'imgui/imgui.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/imgui_demo.cpp',
    'imgui/backends/imgui_impl_glfw.cpp',
    'imgui/backends/imgui_impl_vulkan.cpp'
    ],
  include_directories : inc_imgui
  )
