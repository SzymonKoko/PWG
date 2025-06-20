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
    if (input->IsPressed(Action::Exit)) //256 - ESCAPE
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
    float lastFrame{ 0.0f };
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
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
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // Creating window
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "PWG", nullptr, nullptr);
    if (!m_window) {
        Logger::LogError(Logger::Module::Window, "Failed to create GLFW window");
        glfwTerminate();
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
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
}

void pwg::Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
