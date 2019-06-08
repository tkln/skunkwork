#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif // _WIN32

#include <GL/gl3w.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"

#include "gpuProfiler.hpp"
#include "gui.hpp"
#include "marched.hpp"
#include "quad.hpp"
#include "shader.hpp"
#include "timer.hpp"
#include "window.hpp"

using namespace glm;

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
    Marched m;

    // Set up scene
    std::string vertPath(RES_DIRECTORY);
    vertPath += "shader/basic_vert.glsl";
    std::string fragPath(RES_DIRECTORY);
    fragPath += "shader/basic_frag.glsl";
    //Shader shader(vertPath, fragPath, "");
    vertPath = RES_DIRECTORY;
    vertPath += "shader/tri_vert.glsl";
    fragPath = RES_DIRECTORY;
    fragPath += "shader/tri_frag.glsl";
    Shader triShader(vertPath, fragPath, "");

    Timer reloadTime;
    Timer globalTime;
    GpuProfiler sceneProf(5);
    std::vector<std::pair<std::string, const GpuProfiler*>> profilers =
        {{"Scene", &sceneProf}};

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2, 0.2, 0.2, 1.0);
    vec3 cameraPos(0, 0, 3);
    // Run the main loop
    int t = 0;
    while (window.open()) {
        window.startFrame();

        if (window.drawGUI())
            gui.startFrame(window.height(), triShader.dynamicUniforms(), profilers);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Try reloading the shader every 0.5s
        if (reloadTime.getSeconds() > 0.5f) {
            //shader.reload();
            triShader.reload();
            reloadTime.reset();
        }

        // TODO: No need to reset before switch back
        if (gui.useSliderTime())
            globalTime.reset();

        ImGui::Begin("HAX");
        ImGui::DragFloat3("campos", (float*)&cameraPos, 0.01f);
        ImGui::End();

        mat4 modelToWorld = mat4(1);
        mat3 normalToWorld = mat3(transpose(inverse(modelToWorld)));
        mat4 worldToClip =
            perspective(45.f, window.width() / float(window.height()), 0.01f, 10.f) *
            lookAt(cameraPos, vec3(0, 0, 0), vec3(0, 1, 0));

        sceneProf.startSample();
        m.update(uvec3(50), vec3(-5), vec3(5), 0.f);

        triShader.bind();
        glUniformMatrix4fv(glGetUniformLocation(triShader._progID, "uModelToWorld"), 1, false, (GLfloat*) &modelToWorld);
        glUniformMatrix3fv(glGetUniformLocation(triShader._progID, "uNormalToWorld"), 1, false, (GLfloat*) &normalToWorld);
        glUniformMatrix4fv(glGetUniformLocation(triShader._progID, "uWorldToClip"), 1, false, (GLfloat*) &worldToClip);
        glUniform3fv(glGetUniformLocation(triShader._progID, "uCameraToClip"), 1, (GLfloat*) &cameraPos);
        m.render();
        sceneProf.endSample();

        if (window.drawGUI())
            gui.endFrame();

        window.endFrame();
        ++t;
    }

    gui.destroy();
    window.destroy();
    exit(EXIT_SUCCESS);
}
