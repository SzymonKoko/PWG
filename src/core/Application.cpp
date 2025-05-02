#include <core/Application.h>

pwg::Application::Application()
{
    InitApplication();
}

pwg::Application::~Application()
{
}

void pwg::Application::InitApplication()
{
    
}

void pwg::Application::Run()
{
    // G³ówna pêtla
    while (!m_window.WindowShouldClose()) {
        // Kolor t³a
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        m_window.SwapBuffers();
        m_window.PollEvents();
    }
}
