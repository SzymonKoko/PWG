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

    Shader shaderProgram("../assets/shaders/default.vert", "../assets/shaders/default.frag");
    std::vector <GLfloat> verts(vertices, vertices + sizeof(vertices) / sizeof(GLfloat));
    std::vector <GLuint> ind(indices, indices + sizeof(indices) / sizeof(GLuint));

    std::cout << "verts: " << verts.size() << "   ind: " << ind.size();

    Mesh mesh(verts, ind);
    // G³ówna pêtla
    while (!m_window.WindowShouldClose()) {
        // Kolor t³a
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram.ActivateShader();
        mesh.Draw(shaderProgram);

        m_window.SwapBuffers();
        m_window.PollEvents();
    }
}
