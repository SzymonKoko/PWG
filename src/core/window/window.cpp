#include <glad/glad.h>
#include "window.h"

pwg::Window::Window(MouseInput& mouseInput, KeyboardInput& keyboardInput)
    : m_mouseInput(mouseInput),
      m_keyboardInput(keyboardInput),
      m_window(nullptr)
{
    // GLFW Init
    if (!glfwInit()) {
        Logger::LogError(Logger::Module::Window, "Failed to initialize GLFW");
        return;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (!mode) {
        Logger::LogError(Logger::Module::Window, "Failed to get video mode!");
        glfwTerminate();
        return;
    }
    m_windowWidth = mode->width;
    m_windowHeight = mode->height;

    // Tworzenie okna
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "PWG", nullptr, nullptr);
    if (!m_window) {
        Logger::LogError(Logger::Module::Window, "Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    else
    {
        Logger::LogInfo(Logger::Module::Window, "Window created");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    glfwSetScrollCallback(m_window, scroll_callback);

    // GLAD Init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::LogError(Logger::Module::Window, "Failed to initialize GLAD");
    }

    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetWindowUserPointer(m_window, this);
}

pwg::Window::~Window()
{
    glfwDestroyWindow(m_window);
    Logger::LogInfo(Logger::Module::Window, "Window destroyed");
}

void pwg::Window::Update(float dt)
{
    PollEvents();
    UpdateDeltaTime();
}

bool pwg::Window::WindowShouldClose() const
{
    if (m_keyboardInput.IsPressed(Action::Exit))
    {
        glfwSetWindowShouldClose(m_window, true);
    }
    return glfwWindowShouldClose(m_window);
}

void pwg::Window::SetWindowSize(float width, float height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

void pwg::Window::PollEvents()
{
    glfwPollEvents();
}

void pwg::Window::SwapBuffers()
{
    glfwSwapBuffers(m_window);
}

void pwg::Window::UpdateDeltaTime()
{
    float currentFrame = glfwGetTime();
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
}

void pwg::Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void pwg::Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto s_window = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (s_window)
    {
        s_window->m_mouseInput.SetScrollOffset(xoffset, yoffset);
    }
}
