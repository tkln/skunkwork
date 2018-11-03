#include "gui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 410");
    ImGui::StyleColorsDark();
}

void GUI::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Tweak
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Always);
    ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Once);
    ImGui::Begin("Tweak");
    ImGui::Checkbox("Slider time", &_useSliderTime);
    ImGui::SliderFloat("Time", &_sliderTime, 0.f, 150.f);
    ImGui::End();

    // Log
    ImGui::SetNextWindowSize(ImVec2(LOGW, LOGH), ImGuiSetCond_Always);
    ImGui::SetNextWindowPos(ImVec2(LOGM, windowHeight - LOGH - LOGM), ImGuiSetCond_Always);
    ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
