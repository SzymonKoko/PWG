#include <glad/glad.h>
#include "Application.h"


pwg::Application::Application()
{
    Logger::Init();

    m_keyboardInput = std::make_unique<KeyboardInput>(nullptr);
    m_mouseInput = std::make_unique<MouseInput>(nullptr);
    m_window = std::make_unique<Window>(*m_mouseInput, *m_keyboardInput);
    // Needs the GL context created by the window (GPU query objects).
    m_debugView = std::make_unique<debug::DebugView>(*m_window);

    m_keyboardInput->SetWindow(m_window->GetWindow());
    m_mouseInput->SetWindow(m_window->GetWindow());

    m_resourceManager = std::make_shared<ResourceManager>();
    m_renderer = std::make_shared<Renderer>();

    m_scene = std::make_shared<SceneManager>(m_window->GetWindow(), *m_mouseInput, *m_keyboardInput, m_resourceManager, *m_renderer);
    m_gui = std::make_unique<Gui>(*m_window, m_scene);

    m_window->SetFramerateLimit(120);
    m_window->EnableVSync(false);

    PWG_INFO("Application initialized");
}

pwg::Application::~Application()
{
    m_debugView->Shutdown();
    m_resourceManager->UnloadAll();
    glfwTerminate();
    PWG_INFO("Application destroyed");
}

void pwg::Application::Update()
{
    debug::FramePhaseTimer updateTimer(debug::FramePhase::UPDATE);
    m_window->Update(m_window->GetDeltaTime());
    m_keyboardInput->Update();
    m_mouseInput->Update();
    // Gui::Update runs the single scene update of the frame (welcome screen and editor).
    {
        PWG_CPU_SCOPE("Gui::Update");
        m_gui->Update(m_window->GetDeltaTime());
    }
}

void pwg::Application::Render()
{
    {
        // cpu_render_ms excludes SwapBuffers, which Window::SwapBuffers times as its own phase.
        debug::FramePhaseTimer renderTimer(debug::FramePhase::RENDER);
        PWG_CPU_SCOPE("Gui::Render");
        m_gui->Render();
    }
    m_window->SwapBuffers();
}

void pwg::Application::Run()
{
    //Main loop
    while (!m_window->WindowShouldClose()) {
        m_debugView->BeginFrame();
        Update();
        Render();

        float frameLimit = m_window->GetFrameTimeLimit();
        if (frameLimit > 0.0f && m_window->GetDeltaTime() < frameLimit)
        {
            debug::FramePhaseTimer sleepTimer(debug::FramePhase::SLEEP);
            std::this_thread::sleep_for(std::chrono::duration<double>(frameLimit - m_window->GetDeltaTime()));
        }
        m_debugView->EndFrame();
    }
}
