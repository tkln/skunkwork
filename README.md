# skunkwork
A lightweight framework for fooling around with GLSL-shaders, mainly designed for
ray marching. Current features:
  * Includes in glsl
  * Log window with correctly parsed shader errors even with includes
  * Auto-reloading shaders when sources are saved
  * Mercury's [hg_sdf](http://mercury.sexy/hg_sdf) included for CSG
  * Music playback via singleton using BASS
    * Interface for Rocket

I have used [emoon's version](https://github.com/emoon/rocket) as my Rocket-server.

## Dependencies
Building skunkwork requires OpenGL and [GLFW3](http://www.glfw.org) with CMake
being able to find them. BASS is also required and it can be downloaded [here](https://www.un4seen.com/bass.html).
Just drop the header and dylib/so/lib into `ext/bass/`. [dear imgui](https://github.com/ocornut/imgui),
[librocket](https://github.com/rocket/rocket) and [pre-generated gl3w](https://github.com/sndels/libgl3w)
are provided as submodules.

## Building
The CMake-build should work^tm on Sierra (make + AppleClang), Linux (clang) and
Windows 10 (Visual Studio 2017). Submodules need be pulled before running cmake:
```
git submodule init
git submodule update
```
