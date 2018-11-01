#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif // _WIN32

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <iostream>
#include <sstream>

#include "logger.hpp"
#include "gpuProfiler.hpp"
#include "quad.hpp"
#include "shaderProgram.hpp"
#include "timer.hpp"
#include "window.hpp"

#define GUI

using std::cout;
using std::cerr;
using std::endl;

namespace {
    float LOGW = 690.f;
    float LOGH = 210.f;
    float LOGM = 10.f;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    (void) hInstance;
    (void) hPrevInstance;
    (void) lpCmdLine;
    (void) nCmdShow;
#else
int main()
{
#endif // _WIN32
    // Init GLFW-context
    Window window;
    if (!window.init(1280, 720, "skunktoy"))
        return -1;

#ifdef GUI
    // Setup imgui
    ImGui_ImplGlfwGL3_Init(window.ptr(), true);
    ImGuiWindowFlags logWindowFlags= 0;
    logWindowFlags |= ImGuiWindowFlags_NoTitleBar;
    logWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
    bool showLog = true;

    Logger logger;
    logger.AddLog("[gl] Context: %s\n     GLSL: %s\n",
                   glGetString(GL_VERSION),
                   glGetString(GL_SHADING_LANGUAGE_VERSION));


    // Capture cout for logging
    std::stringstream logCout;
    std::streambuf* oldCout = std::cout.rdbuf(logCout.rdbuf());
#endif // GUI

    Quad q;

    // Set up scene
    std::string vertPath(RES_DIRECTORY);
    vertPath += "shader/basic_vert.glsl";
    std::string fragPath(RES_DIRECTORY);
    fragPath += "shader/basic_frag.glsl";
    ShaderProgram shader(vertPath, fragPath, "");

    Timer reloadTime;
    Timer globalTime;
    GpuProfiler sceneProf(5);

    // Run the main loop
    while (window.open()) {
        window.startFrame();

#ifdef GUI
        ImGui_ImplGlfwGL3_NewFrame();
#endif // GUI

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GUI
        // Update imgui
        {
            ImGui::SetNextWindowSize(ImVec2(LOGW, LOGH), ImGuiSetCond_Once);
            ImGui::SetNextWindowPos(ImVec2(LOGM, window.height() - LOGH - LOGM), ImGuiSetCond_Always);
            ImGui::Begin("Log", &showLog, logWindowFlags);
            ImGui::Text("Frame: %.1f Scene: %.1f", 1000.f / ImGui::GetIO().Framerate, sceneProf.getAvg());
            if (logCout.str().length() != 0) {
                logger.AddLog("%s", logCout.str().c_str());
                logCout.str("");
            }
            logger.Draw();
            ImGui::End();
        }
#endif // GUI

        // Try reloading the shader every 0.5s
        if (reloadTime.getSeconds() > 0.5f) {
            shader.reload();
            reloadTime.reset();
        }

        sceneProf.startSample();
        shader.bind();
        glUniform1f(shader.getULoc("uTime"), globalTime.getSeconds());
        GLfloat res[] = {static_cast<GLfloat>(window.width()), static_cast<GLfloat>(window.height())};
        glUniform2fv(shader.getULoc("uRes"), 1, res);
        q.render();
        sceneProf.endSample();

#ifdef GUI
        ImGui::Render();
#endif // GUI

        window.endFrame();
    }

#ifdef GUI
    std::cout.rdbuf(oldCout);
    ImGui_ImplGlfwGL3_Shutdown();
#endif // GUI

    window.destroy();
    exit(EXIT_SUCCESS);
}
