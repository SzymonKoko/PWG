#include <glad.h>
#include "window.h"

pwg::Window::Window()
{
    InitWindow();
}

pwg::Window::~Window()
{
    glfwTerminate();
}

void pwg::Window::Update()
{
    PollEvents();
    UpdateDeltaTime();
}

bool pwg::Window::WindowShouldClose(pwg::KeyboardInput* input) const
{
    if (input->IsPressed(Action::Exit))
    {
        glfwSetWindowShouldClose(m_window, true);
    }
    return glfwWindowShouldClose(m_window);
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
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}

void pwg::Window::SetWindowSizeAndPosition(GLFWwindow* window, int newWidth, int newHeight)
{

    const int titleBarHeight = 35;

    if (newWidth > m_screenWidth)
    {
        newWidth = m_screenWidth;
    }
    if (newHeight > m_screenHeight)
    {
        newHeight = m_screenHeight;
    }

    glfwSetWindowSize(window, newWidth, newHeight);

    int xpos, ypos;
    glfwGetWindowPos(window, &xpos, &ypos);

    if (xpos + newWidth > m_screenWidth)
    {
        xpos = m_screenWidth - newWidth;
    }

    if (ypos + newHeight > m_screenHeight)
    {
        ypos = m_screenHeight - newHeight;
    }

    if (xpos < 0)
    {
        xpos = 0;
    }
    if (ypos < titleBarHeight)
    {
        ypos = titleBarHeight; 
    }

    glfwSetWindowPos(window, xpos, ypos);
}


float pwg::Window::GetDeltaTime()
{
    return deltaTime;
}

void pwg::Window::InitWindow()
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

    // Pobranie rozdzielczoœci ekranu
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (!mode) {
        Logger::LogError(Logger::Module::Window, "Failed to get video mode!");
        glfwTerminate();
        return;
    }
    m_screenWidth = mode->width;
    m_screenHeight = mode->height;

    // Ograniczenie rozmiaru okna do rozdzielczoœci ekranu
    if (m_windowWidth > m_screenWidth) m_windowWidth = m_screenWidth;
    if (m_windowHeight > m_screenHeight) m_windowHeight = m_screenHeight;

    // Tworzenie okna
    m_window = glfwCreateWindow(m_screenWidth, m_screenHeight, "PWG", nullptr, nullptr);
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

    // GLAD Init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::LogError(Logger::Module::Window, "Failed to initialize GLAD");
    }

    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void pwg::Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
