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

#include "log.hpp"

#include <GL/gl3w.h>

Log& Log::instance()
{
    static Log log;
    return log;
}

void Log::clear()
{
    _buf.clear();
    _lineOffsets.clear();
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

Log::Log()
{
    // Start log with GL context info
    addLog("[gl] Context: %s\n", glGetString(GL_VERSION));
    addLog("[gl] GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void GUILog::clear()
{
    Log::instance().clear();
}

void GUILog::draw()
{
    Log::instance().draw();
}
