# skunkwork
A lightweight framework for fooling around with GLSL-shaders. Mainly designed for
ray marching.

## Dependencies
Building skunkwork requires OpenGL and [GLFW3](http://www.glfw.org) with CMake
being able to find them. [dear imgui](https://github.com/ocornut/imgui) and
[pre-generated gl3w](https://github.com/sndels/libgl3w) are provided as submodules.

## Building
The CMake-build should work^tm on Sierra (make + AppleClang, Xcode) and Windows 10
(Visual Studio 2017). Submodules need be pulled before running cmake:
```
git submodule init
git submodule update
```
