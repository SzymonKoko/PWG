#include "cameraManager.h"

pwg::CameraManager::CameraManager(pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
	: m_mouseInput(mouseInput),
	  m_keyboardInput(keyboardInput)
{
	m_editorCamera = std::make_shared<EditorCamera>();
	m_playerCamera = std::make_shared<PlayerCamera>();
	m_activeCamera = m_editorCamera;
	m_activeCameraType = CameraType::EDITOR;
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
		m_activeCameraType = CameraType::EDITOR;
		PWG_INFO("SET CAMERA TO EDITOR MODE");
		break;
	}
	case CameraType::PLAYER:
	{
		m_activeCamera = m_playerCamera;
		m_activeCameraType = CameraType::PLAYER;
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

void pwg::CameraManager::SetCameraPosition(float x, float y, float z)
{
	if (m_activeCamera)
	{
		m_activeCamera->SetCameraPosition(x, y, z);
	}
}

glm::vec3 pwg::CameraManager::GetCameraPosition()
{
	return m_activeCamera->GetCameraPosition();
}

std::shared_ptr<pwg::ICamera> pwg::CameraManager::GetActiveCamera()
{
	return m_activeCamera;
}

pwg::CameraType& pwg::CameraManager::GetActiveCameraType()
{
	return m_activeCameraType;
}