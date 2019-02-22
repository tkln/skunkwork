#include "gui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "log.hpp"

namespace {
    float LOGW = 690.f;
    float LOGH = 210.f;
    float LOGM = 10.f;

    inline void uniformOffset()
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 27.f);
    }
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
    std::unordered_map<std::string, Uniform>& uniforms,
    const std::vector<std::pair<std::string, const GpuProfiler*>>& timers
)
{
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Uniform editor
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_Once);
    ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Once);
    ImGui::Begin("Uniform Editor");
    ImGui::Checkbox("##Use slider time", &_useSliderTime);
    ImGui::SameLine(); ImGui::SliderFloat("uTime", &_sliderTime, 0.f, 1.f);
    for (auto& e : uniforms) {
        std::string name = e.first;
        Uniform& uniform = e.second;
        switch (uniform.type) {
        case UniformType::Float:
            uniformOffset();
            ImGui::DragFloat(name.c_str(), uniform.value, 0.01f);
            break;
        case UniformType::Vec2:
            uniformOffset();
            ImGui::DragFloat2(name.c_str(), uniform.value, 0.01f);
            break;
        case UniformType::Vec3:
            ImGui::ColorEdit3(
                std::string("##" + name).c_str(),
                uniform.value,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel
            );
            ImGui::SameLine(); ImGui::DragFloat3(name.c_str(), uniform.value, 0.01f);
            break;
        default:
            ADD_LOG("[gui] Unknown dynamic uniform type\n");
            break;
        }
    }
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
