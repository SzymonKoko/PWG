#include "cameraManager.h"

pwg::CameraManager::CameraManager(pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
	: m_mouseInput(mouseInput),
	  m_keyboardInput(keyboardInput)
{
	m_editorCamera = std::make_shared<EditorCamera>();
	m_activeCamera = m_editorCamera;
}

void pwg::CameraManager::Update(float dt)
{
	if (m_activeCamera)
	{
		m_activeCamera->Update(dt, m_mouseInput, m_keyboardInput);
	}
}

void pwg::CameraManager::SetCamera(CameraType type)
{
	switch (type)
	{
	case CameraType::EDITOR:
	{
		m_activeCamera = m_editorCamera;
		PWG_INFO("SET CAMERA TO EDITOR MODE");
		break;
	}
	case CameraType::PLAYER:
	{
		PWG_INFO("SET CAMERA TO PLAYER MODE");
		break;
	}
	default:
	{
		PWG_ERROR("WRONG CAMERA TYPE");
	}
	}
}

void pwg::CameraManager::SetDefaultCameraPosition(int size)
{
	if (m_activeCamera)
	{
		m_activeCamera->SetDefaultCameraPosition(size);
	}
}

std::shared_ptr<pwg::ICamera> pwg::CameraManager::GetActiveCamera()
{
	return m_activeCamera;
}
