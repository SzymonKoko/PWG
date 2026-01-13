#include "playerCamera.h"

pwg::PlayerCamera::PlayerCamera()
{
	m_cameraProperties.up = { 0.0f, 1.7f, 0.0f };
}

void pwg::PlayerCamera::Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
{
	HandleMouseInput(mouseInput, dt);
	HandleKeyboardInput(keyboardInput, dt);

	m_cameraProperties.projectionMatrix = glm::perspective(glm::radians(m_cameraProperties.fov), m_cameraProperties.aspectRatio, m_cameraProperties.nearPlane, m_cameraProperties.farPlane);
}

void pwg::PlayerCamera::SetDefaultCameraPosition(int size)
{
	m_transform.position = { 0.0f, 500.0f, 0.0f };
}

void pwg::PlayerCamera::SetCameraPosition(float x, float y, float z)
{
	m_transform.position = { x, y, z };
}

glm::mat4& pwg::PlayerCamera::GetViewMatrix()
{
	return m_cameraProperties.viewMatrix;
}

glm::mat4& pwg::PlayerCamera::GetProjectionMatrix()
{
	return m_cameraProperties.projectionMatrix;
}
bool pwg::PlayerCamera::IsSpawned()
{
	return m_spawned;
}

void pwg::PlayerCamera::SetSpawned(bool spawned)
{
	m_spawned = spawned;
}

glm::vec3 pwg::PlayerCamera::GetCameraPosition()
{
	return m_transform.position;
}

void pwg::PlayerCamera::HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt)
{
	glm::vec3 cameraRight = glm::normalize(glm::cross(m_cameraProperties.forward, m_cameraProperties.up));

	if (keyboardInput.IsHeld(Action::MoveForward))
	{
		m_transform.position += m_playerCameraProperties.cameraSpeed * m_cameraProperties.forward;
	}
	if (keyboardInput.IsHeld(Action::MoveBackward))
	{
		m_transform.position -= m_playerCameraProperties.cameraSpeed * m_cameraProperties.forward;
	}
	if (keyboardInput.IsHeld(Action::MoveLeft))
	{
		m_transform.position -= m_playerCameraProperties.cameraSpeed * cameraRight;
	}
	if (keyboardInput.IsHeld(Action::MoveRight))
	{
		m_transform.position += m_playerCameraProperties.cameraSpeed * cameraRight;
	}

	m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_transform.position + m_cameraProperties.forward, m_cameraProperties.up);
}

void pwg::PlayerCamera::HandleMouseInput(pwg::MouseInput& mouseInput, float dt)
{
	float deltaX = mouseInput.GetDeltaX();
	float deltaY = mouseInput.GetDeltaY();

	//Moving m_cameraProperties with mouse
	float xoffset = deltaX * m_playerCameraProperties.cameraSensitivity;
	float yoffset = deltaY * m_playerCameraProperties.cameraSensitivity;

	//Updating yaw and pitch angles
	m_playerCameraProperties.yaw += xoffset;
	m_playerCameraProperties.pitch += yoffset;
	m_playerCameraProperties.pitch = glm::clamp(m_playerCameraProperties.pitch, -89.0f, 89.0f);

	CalculateCameraForward();

	m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_transform.position + m_cameraProperties.forward, m_cameraProperties.up);
}

void pwg::PlayerCamera::CalculateCameraForward()
{
	float yawRad = glm::radians(m_playerCameraProperties.yaw);
	float pitchRad = glm::radians(m_playerCameraProperties.pitch);

	m_cameraProperties.forward = glm::normalize(glm::vec3(cos(pitchRad) * cos(yawRad), sin(pitchRad), cos(pitchRad) * sin(yawRad)));
}
