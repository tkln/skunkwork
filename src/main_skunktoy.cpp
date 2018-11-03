#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif // _WIN32

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include "gpuProfiler.hpp"
#include "gui.hpp"
#include "quad.hpp"
#include "shaderProgram.hpp"
#include "timer.hpp"
#include "window.hpp"

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

    // Setup imgui
    GUI gui;
    gui.init(window.ptr());

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
    std::vector<std::pair<std::string, const GpuProfiler*>> profilers =
        {{"Scene", &sceneProf}};

    // Run the main loop
    while (window.open()) {
        window.startFrame();

        if (window.drawGUI())
            gui.startFrame(window.height(), profilers);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Try reloading the shader every 0.5s
        if (reloadTime.getSeconds() > 0.5f) {
            shader.reload();
            reloadTime.reset();
        }

        sceneProf.startSample();
        shader.bind();
        glUniform1f(shader.getULoc("uTime"),
                    gui.useSliderTime() ? gui.sliderTime() : globalTime.getSeconds());
        GLfloat res[] = {static_cast<GLfloat>(window.width()), static_cast<GLfloat>(window.height())};
        glUniform2fv(shader.getULoc("uRes"), 1, res);
        q.render();
        sceneProf.endSample();

        if (window.drawGUI())
            gui.endFrame();

        window.endFrame();
    }

    gui.destroy();
    window.destroy();
    exit(EXIT_SUCCESS);
}
