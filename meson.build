project('eldr', 'cpp',
  version : '0.0.1',
  license : 'MIT',
  default_options : [
    'cpp_std=c++20',
    'buildtype=debug',
    'warning_level=2',    # -Wall -Wextra
    'b_ndebug=if-release',
    'default_library=static'
    ]
  )

add_global_arguments(
    #'-fmodules-ts',       # Needed to get module support in GCC (didn't work, try in future)
    language : 'cpp'
    )

# Meson will use -O3 if buildtype=release
if get_option('buildtype').startswith('debugoptimized')
  add_project_arguments(
    '-D_DEBUG',
    '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO',
    language : 'cpp'
    )
elif get_option('buildtype').startswith('debug')
  add_project_arguments(
    '-D_DEBUG',
    '-DELDR_VULKAN_DEBUG_REPORT',
    '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG',
    language : 'cpp'
    )
elif get_option('buildtype').startswith('release')
  add_project_arguments(
    '-DNDEBUG',
    '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN',
    language : 'cpp'
    )
endif  

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------
libglfw       = dependency('glfw3',   version: '>=3.3.2')
libvulkan     = dependency('vulkan',  version: '>=1.2.131')
libjpeg       = dependency('libjpeg', version: '>=2.0.3')
libpng        = dependency('libpng',  version: '>=1.6.3')
libspdlog     = dependency('spdlog',  version: '>=1.5.0')

# ------------------------------------------------------------------------------
# The rest
# ------------------------------------------------------------------------------
include = include_directories('include')
subdir('external')
subdir('src')
