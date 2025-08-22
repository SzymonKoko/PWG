#include "keyboardInput.h"

pwg::KeyboardInput::KeyboardInput(GLFWwindow* window)
	:m_window(window)
{
	m_keyBindings[static_cast<int>(Action::MoveForward)] = GLFW_KEY_W;
	m_keyBindings[static_cast<int>(Action::MoveBackward)] = GLFW_KEY_S;
	m_keyBindings[static_cast<int>(Action::MoveLeft)] = GLFW_KEY_A;
	m_keyBindings[static_cast<int>(Action::MoveRight)] = GLFW_KEY_D;
	m_keyBindings[static_cast<int>(Action::MoveUp)] = GLFW_KEY_SPACE;
	m_keyBindings[static_cast<int>(Action::MoveDown)] = GLFW_KEY_LEFT_SHIFT;
	m_keyBindings[static_cast<int>(Action::Exit)] = GLFW_KEY_ESCAPE;
	m_keyBindings[static_cast<int>(Action::ToggleDebug)] = GLFW_KEY_F1;

	Logger::LogInfo(Logger::Module::KeyboardInput, "Keyboard ready");
}

void pwg::KeyboardInput::Update()
{
	for (int i = 0; i < NumActions; ++i)
	{
		m_previousState[i] = m_currentState[i];
		int key = m_keyBindings[i];
		m_currentState[i] = glfwGetKey(m_window, key) == GLFW_PRESS;
	}
}

bool pwg::KeyboardInput::IsPressed(Action action) const
{
	int i = static_cast<int>(action);
	return m_currentState[i] && !m_previousState[i];
}

bool pwg::KeyboardInput::IsHeld(Action action) const
{
	return m_currentState[static_cast<int>(action)];
}

bool pwg::KeyboardInput::IsReleased(Action action) const
{
	int i = static_cast<int>(action);
	return !m_currentState[i] && m_previousState[i];
}

void pwg::KeyboardInput::RebindKey(Action action, int newKey)
{
	m_keyBindings[static_cast<int>(action)] = newKey;
}

int pwg::KeyboardInput::GetKeyForAction(Action action) const
{
	return m_keyBindings[static_cast<int>(action)];
}

void pwg::KeyboardInput::SetWindow(GLFWwindow* window)
{
	m_window = window;
}

