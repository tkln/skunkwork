
#ifndef SKUNKWORK_GUI_HPP
#define SKUNKWORK_GUI_HPP

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <sstream>
#include <utility>
#include <vector>

#include "gpuProfiler.hpp"

/*
Log is based on dear imgui's ExampleAppLog under the following license

The MIT License (MIT)

Copyright (c) 2014-2015 Omar Cornut and ImGui contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
class Log
{
public:
    Log();
    ~Log();

    void clear();
    void addLog(const char* fmt, ...) IM_PRINTFARGS(2);
    void draw();

private:
    ImGuiTextBuffer _buf;
    ImGuiTextFilter _filter;
    ImVector<int> _lineOffsets;
    std::stringstream _logCout;
    std::streambuf* _oldCout;
    bool _scrollToBottom;
};

class GUI
{
public:
    GUI();

    void init(GLFWwindow* window);
    void destroy();
    bool useSliderTime() const;
    float sliderTime() const;

    void startFrame(int windowHeight,
                    const std::vector<std::pair<std::string, const GpuProfiler*>>& timers);
    void endFrame();

private:
    Log _log;
    bool _useSliderTime;
    float _sliderTime;
};

#endif // SKUNKWORK_GUI_HPP
