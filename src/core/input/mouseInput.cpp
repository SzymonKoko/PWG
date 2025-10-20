#include "mouseInput.h"

pwg::MouseInput::MouseInput(GLFWwindow* window)
	:m_window(window)
{
    PWG_INFO("Mouse ready");
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

void pwg::MouseInput::SetScrollOffset(double xoffset, double yoffset)
{
    m_scrollOffsetX = xoffset;
    m_scrollOffsetY = yoffset;
}

double pwg::MouseInput::GetScrollOffsetY()
{
    double scroll = m_scrollOffsetY;
    ResetScrollOffset();
    return scroll;
}

void pwg::MouseInput::ResetScrollOffset()
{
    m_scrollOffsetX = 0.0f;
    m_scrollOffsetY = 0.0f;
}

void pwg::MouseInput::SetWindow(GLFWwindow* window)
{
    m_window = window;
}
