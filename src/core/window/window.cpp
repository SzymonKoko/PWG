#include <glad/glad.h>
#include "window.h"


pwg::Window::Window(MouseInput& mouseInput, KeyboardInput& keyboardInput)
    : m_mouseInput(mouseInput),
      m_keyboardInput(keyboardInput),
      m_window(nullptr)
{
    // GLFW Init
    if (!glfwInit()) {
        PWG_ERROR("Failed to initialize GLFW");
        return;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (!mode) {
        PWG_ERROR("Failed to get video mode!");
        glfwTerminate();
        return;
    }
    m_windowWidth = mode->width;
    m_windowHeight = mode->height;

    // Tworzenie okna
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "PWG", nullptr, nullptr);
    if (!m_window) {
        PWG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    else
    {
        PWG_INFO("Window created");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    glfwSetScrollCallback(m_window, scroll_callback);

    // GLAD Init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        PWG_ERROR("Failed to initialize GLAD");
    }

    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetWindowUserPointer(m_window, this);
}

pwg::Window::~Window()
{
    glfwDestroyWindow(m_window);
    PWG_INFO("Window destroyed");
}

void pwg::Window::Update(float dt)
{
    PollEvents();
    UpdateDeltaTime();
    CountFPS(m_deltaTime);
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

void pwg::Window::SetFramerateLimit(unsigned int limit)
{
    if (limit > 0)
    {
        m_frameTimeLimit = 1.0f / (float)limit;
    }
    else
    {
        m_frameTimeLimit = 0.0f;
    }
}

void pwg::Window::EnableVSync(bool enabled)
{
    if (enabled)
    {
        glfwSwapInterval(1);
    }
    else
    {
        glfwSwapInterval(0);
    }
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

void pwg::Window::CountFPS(const float& dt)
{
    static float timer = 0.0f;
    static int frames = 0;
    timer += dt;
    frames++;

    if (timer >= 1.0f) // co 1 sekunda
    {
        float fps = frames / timer;

        std::stringstream ss;
        ss << "PWG v0.5 [" << static_cast<int>(fps) << " FPS]";
        glfwSetWindowTitle(m_window, ss.str().c_str());

        frames = 0;
        timer = 0.0f;
    }
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
