#include "sceneManager.h"

pwg::SceneManager::SceneManager(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput)
	: m_window(window),
	  m_mouseInput(minput),
	  m_keyboardInput(kinput)
{
	m_editorScene = std::make_unique<pwg::EditorScene>(m_window, m_mouseInput, m_keyboardInput);
	m_currentScene = m_editorScene->Clone();
}

void pwg::SceneManager::Play()
{
	auto playScene = std::make_unique<pwg::PlayScene>();
	playScene->LoadFromEditorScene(*m_editorScene);
	m_currentScene = std::move(playScene);
}

void pwg::SceneManager::Stop()
{
	m_currentScene = m_editorScene->Clone();
}

void pwg::SceneManager::Update(const float& dt)
{
	if (m_editorScene)
	{
		m_editorScene->Update(dt);
	}
}

void pwg::SceneManager::Draw()
{
	if (m_editorScene)
	{
		m_editorScene->Draw();
	}
}
