#include "mouseInput.h"

pwg::MouseInput::MouseInput(GLFWwindow* window)
	:m_window(window)
{

}

void pwg::MouseInput::Update()
{
	

    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);

    if (m_firstClick)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstClick = false;
    }

    m_deltaX = static_cast<float>(xpos - m_lastX);
    m_deltaY = static_cast<float>(m_lastY - ypos); // y odwrócone dla typowego systemu kamer

    m_lastX = xpos;
    m_lastY = ypos;
	
}

bool pwg::MouseInput::IsButtonDown(int button) const
{
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}