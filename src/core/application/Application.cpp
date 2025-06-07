#include "Application.h"

pwg::Application::Application()
{
    InitApplication();
    
}

pwg::Application::~Application()
{
    shaderProgram->DeleteShader();
}

void pwg::Application::InitApplication()
{
    shaderProgram = new Shader("../assets/shaders/default.vert", "../assets/shaders/default.frag");
    m_keyboardInput = new KeyboardInput(m_window.GetWindow());
    m_mouseInput = new MouseInput(m_window.GetWindow());
}

void pwg::Application::Run()
{

    // G³ówna pêtla
    while (!m_window.WindowShouldClose(m_keyboardInput)) {
        // Kolor t³a
        m_renderer.Clear();

        m_window.UpdateDeltaTime();

        shaderProgram->ActivateShader();
        
        m_window.PollEvents();

        m_keyboardInput->Update();
        m_mouseInput->Update();
        m_pyramidMesh.InitMesh(shaderProgram->GetShaderID());
        m_renderer.Draw(m_pyramidMesh);
        m_camera.UpdateCamera(shaderProgram->GetShaderID());
        m_camera.MoveCamera(m_window.GetWindow(), m_keyboardInput ,m_window.GetDeltaTime(), m_mouseInput);
        m_camera.UpdateRotation(m_mouseInput);

        m_window.SwapBuffers();
    }
}
