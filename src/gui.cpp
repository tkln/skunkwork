#include "gui.hpp"

#include <imgui_impl_glfw_gl3.h>
#include <iostream>

#include "log.hpp"

namespace {
    float LOGW = 690.f;
    float LOGH = 210.f;
    float LOGM = 10.f;
}

GUI::GUI() :
    _useSliderTime(false),
    _sliderTime(0.f)
{ }

void GUI::init(GLFWwindow* window)
{
    ImGui_ImplGlfwGL3_Init(window, false);
}

void GUI::destroy()
{
    ImGui_ImplGlfwGL3_Shutdown();
}

bool GUI::useSliderTime() const
{
    return _useSliderTime;
}

float GUI::sliderTime() const
{
    return _sliderTime;
}

void GUI::startFrame(
    int windowHeight,
    const std::vector<std::pair<std::string, const GpuProfiler*>>& timers
)
{
    ImGui_ImplGlfwGL3_NewFrame();
    ImGuiWindowFlags logWindowFlags = 0;
    logWindowFlags |= ImGuiWindowFlags_NoTitleBar;
    logWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

    // Tweak
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Always);
    ImGui::Begin("Tweak");
    ImGui::Checkbox("Slider time", &_useSliderTime);
    ImGui::SliderFloat("Time", &_sliderTime, 0.f, 150.f);
    ImGui::End();
    // Log
    ImGui::SetNextWindowSize(ImVec2(LOGW, LOGH), ImGuiSetCond_Always);
    ImGui::SetNextWindowPos(ImVec2(LOGM, windowHeight - LOGH - LOGM), ImGuiSetCond_Always);
    ImGui::Begin("Log", nullptr, logWindowFlags);
    ImGui::Text("Frame: %.1f", 1000.f / ImGui::GetIO().Framerate);
    for (auto& t : timers) {
        ImGui::SameLine(); ImGui::Text("%s: %.1f", t.first.c_str(), t.second->getAvg());
    }
    GUILog::draw();
    ImGui::End();
}

void GUI::endFrame()
{
    ImGui::Render();
}
