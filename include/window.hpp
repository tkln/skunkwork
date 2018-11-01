#ifndef SKUNKWORK_WINDOW_HPP
#define SKUNKWORK_WINDOW_HPP

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <string>

class Window
{
public:
    Window() {};
    bool init(int w, int h, const std::string& title);
    void destroy();

    bool open() const;
    GLFWwindow* ptr() const;
    int width() const;
    int height() const;

    void startFrame();
    void endFrame() const;

    static void errorCallback(int error, const char* description);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
    static void charCallback(GLFWwindow* window, unsigned int c);

private:
    GLFWwindow* _window;
    int _w, _h;
};

#endif // SKUNKWORK_WINDOW_HPP
