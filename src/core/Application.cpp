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

}

void pwg::Application::Run()
{


    // G³ówna pêtla
    while (!m_window.WindowShouldClose()) {
        // Kolor t³a
        m_renderer.Clear();

        shaderProgram->ActivateShader();
        m_renderer.Draw(m_triangleMesh);

        m_window.SwapBuffers();
        m_window.PollEvents();
    }
}
