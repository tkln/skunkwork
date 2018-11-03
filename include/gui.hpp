
#ifndef SKUNKWORK_GUI_HPP
#define SKUNKWORK_GUI_HPP

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <string>
#include <utility>
#include <vector>

#include "gpuProfiler.hpp"

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
    bool _useSliderTime;
    float _sliderTime;
};

#endif // SKUNKWORK_GUI_HPP
