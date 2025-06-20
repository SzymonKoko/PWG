#include "Application.h"

pwg::Application::Application()
{
    InitApplication();
}

pwg::Application::~Application()
{
    
}

void pwg::Application::InitApplication()
{
    Logger::LogInfo(Logger::Module::Application, "Application initialized");
    
    m_keyboardInput = new KeyboardInput(m_window.GetWindow());
    m_mouseInput = new MouseInput(m_window.GetWindow());
}

void pwg::Application::Update()
{
    m_window.Update();
    m_renderer.Update();
    m_keyboardInput->Update();
    m_mouseInput->Update();
    m_camera.UpdateCamera(m_window.GetWindow(), m_keyboardInput, m_window.GetDeltaTime(), m_mouseInput, m_renderer.GetShaderProgramID());
}

void pwg::Application::Render()
{
    m_renderer.Draw();
    m_window.SwapBuffers();
}

void pwg::Application::Run()
{

    //Main loop
    while (!m_window.WindowShouldClose(m_keyboardInput)) {
        m_renderer.Clear();

        Update();
        Render();
        
    }
}
