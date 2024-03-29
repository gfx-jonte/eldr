# 🔥Eldr: Physically Based Renderer
## Introduction
_Eldr_ ("fire" in Old Norse) is my new hobby project. After working with
[_Mitsuba 2_](https://github.com/mitsuba-renderer/mitsuba2) in my master's
thesis [project](https://github.com/gfx-jonte/PPG-quadtree-reconstruction),
I was inspired to create something similar on my own to learn more about
computer graphics. In contrast to Mitsuba, I intend to make Eldr more GUI
oriented, making it possible to set up scenes as you go, and provide more
immediate visual feedback like most commercial renderers do. However, I want to
keep the "research spirit" of Mitsuba, making it as modular as I can to allow
swapping out components and trying new rendering techniques. This is essential
for me as the purpose of this project is for me to learn about rendering.

I will try to keep cross platform compatibility in mind while developing Eldr,
but it is not a priority in the beginning. I will be developing on Linux and the
build instructions etc. will therefore be Linux(debian)-oriented to begin with.

## Requirements
The following software is available on Windows as well, so it should be possible
to build there as well. MSVC 2013 is not compatible with
[cxxopts](https://github.com/jarro2783/cxxopts#requirements), however.
```
$ sudo apt-get install build-essential ninja-build meson python3 python3-pip \
    python3-setuptools python3-wheel libspdlog-dev libglfw3-dev libglm-dev \
    libvulkan-dev libjpeg-turbo8
```
For debugging:
```
sudo apt-get install vulkan-validationlayers-dev spirv-tools
```

Note that:
- [spdlog](https://github.com/gabime/spdlog/tree/v1.13.0) is a header only
library but I have been using a compiled library for faster compilation of Eldr.

## Build
### App
```
$ meson setup build
$ cd build
$ ninja
```
### Shaders
I have been using [glslc](https://github.com/google/shaderc) to compile some
basic shaders. Eldr will look for the compiled shaders `vert.spv` and `frag.spv`
in `resources`.
```
glslc src/shaders/shader.vert -o resources/vert.spv
glslc src/shaders/shader.frag -o resources/frag.spv
```
