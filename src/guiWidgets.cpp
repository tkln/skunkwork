#include "guiWidgets.hpp"

#include <cstdio>
#include <imgui.h>

void drawUniformEditor(std::unordered_map<std::string, Uniform>& uniforms)
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Once);
    ImGui::Begin("Uniform Editor");
    for (auto& e : uniforms) {
        std::string name = e.first;
        Uniform& uniform = e.second;
        switch (uniform.type) {
        case UniformType::Float:
            ImGui::DragFloat(name.c_str(), &uniform.value.f, 0.01f);
            break;
        case UniformType::Vec2:
            ImGui::DragFloat2(name.c_str(), uniform.value.vec2, 0.01f);
            break;
        case UniformType::Vec3:
            ImGui::DragFloat3(name.c_str(), uniform.value.vec3, 0.01f);
            break;
        default:
            printf("[gui] Unknown dynamic uniform type\n");
            break;
        }
    }
    ImGui::End();
}
