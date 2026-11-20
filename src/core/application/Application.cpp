#include <glad/glad.h>
#include "Application.h"
#include "imgui.h"


pwg::Application::Application(ApplicationOptions options)
    : m_options(std::move(options))
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
    m_runner = std::make_unique<debug::ExperimentRunner>(debug::ExperimentHost{ *m_window, *m_gui, *m_scene });
    m_debugView->SetExperimentRunner(m_runner.get());

    m_window->SetFramerateLimit(120);
    m_window->EnableVSync(false);

    PWG_INFO("Application initialized");
}

pwg::Application::~Application()
{
    m_runner.reset();
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
        TickExperiment();
    }
}

void pwg::Application::TickExperiment()
{
    ++m_frames;
    if (!m_options.experimentConfig.empty() && !m_experimentStarted && m_frames >= 1)
    {
        // The GL context and the editor scene exist after the first frame (design 8.1).
        m_experimentStarted = true;
        std::string error;
        if (!m_runner->Start(std::filesystem::path(m_options.experimentConfig), std::filesystem::path(m_options.outputFolder),
            m_options.exitWhenDone, error))
        {
            PWG_ERROR("Experiment not started: {}", error);
            m_experimentRejected = true;
            if (m_options.exitWhenDone)
            {
                m_gui->ForceClose();
            }
        }
    }
    // F9 aborts a run, also in the fullscreen main view where the Experiment tab is hidden. ImGui queues the key events,
    // so a short press is not lost between two frames.
    if (m_runner->IsRunning() && ImGui::IsKeyPressed(ImGuiKey_F9, false))
    {
        m_runner->Abort();
    }
    m_runner->Tick();
}

int pwg::Application::GetExitCode() const
{
    if (m_experimentRejected)
    {
        return 1;
    }
    return m_experimentStarted ? m_runner->GetExitCode() : 0;
}
