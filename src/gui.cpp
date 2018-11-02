#include "gui.hpp"

#include <imgui_impl_glfw_gl3.h>
#include <iostream>

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
    _log.draw();
    ImGui::End();
}

void GUI::endFrame()
{
    ImGui::Render();
}

Log::Log()
{
    // Capture cout for logging
    _oldCout = std::cout.rdbuf(_logCout.rdbuf());
    addLog("[gl] Context: %s\n     GLSL: %s\n",
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION));
}

Log::~Log()
{
    std::cout.rdbuf(_oldCout);
}

void Log::clear()
{
    _buf.clear();
    _lineOffsets.clear();
    _logCout.str("");
}

void Log::addLog(const char* fmt, ...)
{
    int old_size = _buf.size();
    va_list args;
    va_start(args, fmt);
    _buf.appendv(fmt, args);
    va_end(args);
    for (int new_size = _buf.size(); old_size < new_size; old_size++)
        if (_buf[old_size] == '\n')
            _lineOffsets.push_back(old_size);
    _scrollToBottom = true;
}

void Log::draw()
{
    if (_logCout.str().length() != 0) {
        addLog("%s", _logCout.str().c_str());
        _logCout.str("");
    }
    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    _filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0,0), false,
                        ImGuiWindowFlags_HorizontalScrollbar);
    if (copy) ImGui::LogToClipboard();

    if (_filter.IsActive()) {
        const char* buf_begin = _buf.begin();
        const char* line = buf_begin;
        for (int line_no = 0; line != NULL; line_no++)
        {
            const char* line_end = (line_no < _lineOffsets.Size) ?
                                    buf_begin + _lineOffsets[line_no] : NULL;
            if (_filter.PassFilter(line, line_end))
                ImGui::TextUnformatted(line, line_end);
            line = line_end && line_end[1] ? line_end + 1 : NULL;
        }
    } else {
        ImGui::TextUnformatted(_buf.begin());
    }
    // Add empty line to fix scrollbar covering the last line
    const char* emptyLine = "";
    ImGui::TextUnformatted(emptyLine, emptyLine);

    if (_scrollToBottom)
        ImGui::SetScrollHere(1.0f);
    _scrollToBottom = false;
    ImGui::EndChild();
}
