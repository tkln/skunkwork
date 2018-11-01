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
#include <sync.h>
#include <track.h>

#include "audioStream.hpp"
#include "logger.hpp"
#include "gpuProfiler.hpp"
#include "quad.hpp"
#include "scene.hpp"
#include "shaderProgram.hpp"
#include "timer.hpp"
#include "window.hpp"

// Comment out to disable autoplay without tcp-Rocket
//#define MUSIC_AUTOPLAY
// Comment out to load sync from files
//#define TCPROCKET
// Comment out to remove gui
#define GUI

using std::cout;
using std::cerr;
using std::endl;

namespace {
    float LOGW = 690.f;
    float LOGH = 210.f;
    float LOGM = 10.f;
}

//Set up audio callbacks for rocket
static struct sync_cb audioSync = {
    AudioStream::pauseStream,
    AudioStream::setStreamRow,
    AudioStream::isStreamPlaying
};

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
    if (!window.init(1280, 720, "skunkwork"))
        return -1;

#ifdef GUI
    // Setup imgui
    ImGui_ImplGlfwGL3_Init(window.ptr(), true);
    ImGuiWindowFlags logWindowFlags= 0;
    logWindowFlags |= ImGuiWindowFlags_NoTitleBar;
    logWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
    bool showLog = true;
    bool showTweak = true;
    bool useSlider = false;
    float sliderTime = 0.f;

    Logger logger;
    logger.AddLog("[gl] Context: %s\n     GLSL: %s\n",
                   glGetString(GL_VERSION),
                   glGetString(GL_SHADING_LANGUAGE_VERSION));


    // Capture cout for logging
    std::stringstream logCout;
    std::streambuf* oldCout = std::cout.rdbuf(logCout.rdbuf());
#endif // GUI

    Quad q;

    // Set up audio
    std::string musicPath(RES_DIRECTORY);
    musicPath += "music/illegal_af.mp3";
    AudioStream::getInstance().init(musicPath, 175.0, 8);
    int32_t streamHandle = AudioStream::getInstance().getStreamHandle();

    // Set up rocket
    sync_device *rocket = sync_create_device("sync");
    if (!rocket) cout << "[rocket] failed to init" << endl;

    // Set up scene
    std::string vertPath(RES_DIRECTORY);
    vertPath += "shader/basic_vert.glsl";
    std::string fragPath(RES_DIRECTORY);
    fragPath += "shader/basic_frag.glsl";
    Scene scene(std::vector<std::string>({vertPath, fragPath}),
                std::vector<std::string>(), rocket);

#ifdef TCPROCKET
    // Try connecting to rocket-server
    int rocketConnected = sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT) == 0;
    if (!rocketConnected)
        cout << "[rocket] failed to connect" << endl;
#endif // TCPROCKET

    // Init rocket tracks here

    Timer reloadTime;
    Timer globalTime;
    GpuProfiler sceneProf(5);

#ifdef MUSIC_AUTOPLAY
    AudioStream::getInstance().play();
#endif // MUSIC_AUTOPLAY

    // Run the main loop
    while (window.open()) {
        window.startFrame();

        // Sync
        double syncRow = AudioStream::getInstance().getRow();

#ifdef TCPROCKET
        // Try re-connecting to rocket-server if update fails
        // Drops all the frames, if trying to connect on windows
        if (sync_update(rocket, (int)floor(syncRow), &audioSync, (void *)&streamHandle))
            sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif // TCPROCKET

#ifdef GUI
        ImGui_ImplGlfwGL3_NewFrame();
#endif // GUI

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GUI
        // Update imgui
        {
            // Tweak
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Always);
            ImGui::Begin("Tweak", &showTweak, 0);
            ImGui::Checkbox("Slider time", &useSlider);
            ImGui::SliderFloat("Time", &sliderTime, 0.f, 150.f);
            ImGui::End();
            // Log
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
            scene.reload();
            reloadTime.reset();
        }

        sceneProf.startSample();
        scene.bind(syncRow);
#ifdef GUI
        glUniform1f(scene.getULoc("uTime"), useSlider ? sliderTime : globalTime.getSeconds());
#else
        glUniform1f(scene.getULoc("uTime"), globalTime.getSeconds());
#endif // GUI
        GLfloat res[] = {static_cast<GLfloat>(window.width()), static_cast<GLfloat>(window.height())};
        glUniform2fv(scene.getULoc("uRes"), 1, res);
        q.render();
        sceneProf.endSample();

#ifdef GUI
        ImGui::Render();
#endif // GUI

        window.endFrame();

#ifdef MUSIC_AUTOPLAY
        if (!AudioStream::getInstance().isPlaying()) glfwSetWindowShouldClose(windowPtr, GLFW_TRUE);
#endif // MUSIC_AUTOPLAY
    }

    // Save rocket tracks
    sync_save_tracks(rocket);

    // Release resources
    sync_destroy_device(rocket);

#ifdef GUI
    std::cout.rdbuf(oldCout);
    ImGui_ImplGlfwGL3_Shutdown();
#endif // GUI

    window.destroy();
    exit(EXIT_SUCCESS);
}
