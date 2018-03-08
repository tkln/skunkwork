# skunkwork
A lightweight framework for fooling around with GLSL-shaders, mainly designed for
demo work. Current features:
  * Includes in glsl
  * Log window with correctly parsed shader errors even with includes
  * Auto-reloading shaders when sources are saved
  * Mercury's [hg_sdf](http://mercury.sexy/hg_sdf) included for CSG
  * Gpu-"profiler"
    * Holds a vector of last n deltas for averaging
    * Timing instances can't be interleaved because GL_TIMESTAMP doesn't work on OSX
  * Music playback via singleton using BASS
  * Rocket-interface (Windows-build chugs if TCPROCKET is defined but rocket isn't open)
  * Wrapper-classes for most of the common resources (Scene, FrameBuffer, Texture etc.)

I have used [emoon's version](https://github.com/emoon/rocket) as my Rocket-server.

## Build targets
There are two builds: `skunkwork` holds all the bells and whistles while `skunktoy`
drops Rocket and BASS integration for more minimalist shader tinkering.

## Dependencies
Building `skunktoy` requires OpenGL dev libraries and `skunkwork` depends on BASS in
addition to those. BASS can be downloaded [here](https://www.un4seen.com/bass.html).
Just drop the header and dylib/so/lib+dll into `ext/bass/` and `ext/bass/lib/`
respectively. [GLFW3](http://www.glfw.org), [dear imgui](https://github.com/ocornut/imgui),
[librocket](https://github.com/rocket/rocket) and
[pre-generated gl3w](https://github.com/sndels/libgl3w) are provided as submodules.

## Building
The CMake-build should work™ on Sierra, Linux and
Windows 10 (Visual Studio 2017) using cmake. Submodules need be pulled before running cmake:
`git submodule update --recursive --init`
