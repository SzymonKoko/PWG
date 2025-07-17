#include <glad/glad.h>
#include "Application.h"


pwg::Application::Application()
{
    Logger::LogInfo(Logger::Module::Application, "Application initialized");

    m_keyboardInput = new KeyboardInput(m_window.GetWindow());
    m_mouseInput = new MouseInput(m_window.GetWindow());

    m_scene = std::make_shared<Scene>(m_window.GetWindow(), m_mouseInput, m_keyboardInput);
    m_gui = std::make_unique<Gui>(m_window, m_scene);
    m_windowEditor = new WindowEditor(m_window);
    m_windowEditor->InitEditor();
}

pwg::Application::~Application()
{
    delete m_windowEditor;
    delete m_keyboardInput;
    delete m_mouseInput;
}

void pwg::Application::Update()
{
    m_window.Update();
    m_keyboardInput->Update();
    m_mouseInput->Update();
    m_gui->Update();
    m_scene->Update(m_window.GetDeltaTime());
}

void pwg::Application::Render()
{
    m_windowEditor->Render();
    m_gui->Render();
    m_window.SwapBuffers();
}

void pwg::Application::Run()
{
    //Main loop
    while (!m_window.WindowShouldClose(m_keyboardInput)) {
        Update();
        Render();
    }
}
