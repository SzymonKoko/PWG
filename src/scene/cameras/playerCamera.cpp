#include "playerCamera.h"
#include <algorithm>

pwg::PlayerCamera::PlayerCamera()
{
	CalculateCameraForward();
	UpdateMatrices();
}

void pwg::PlayerCamera::Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
{
	dt = std::clamp(dt, 0.0f, 0.05f);
	HandleMouseInput(mouseInput, dt);
	HandleKeyboardInput(keyboardInput, dt);
	UpdateMatrices();
}

void pwg::PlayerCamera::SetDefaultCameraPosition(int size)
{
	m_transform.position = { 0.0f, 500.0f, 0.0f };
	m_playerCameraProperties.velocity = { 0.0f, 0.0f, 0.0f };
	UpdateMatrices();
}

void pwg::PlayerCamera::SetCameraPosition(float x, float y, float z)
{
	m_transform.position = { x, y, z };
	m_playerCameraProperties.velocity = { 0.0f, 0.0f, 0.0f };
	UpdateMatrices();
}

void pwg::PlayerCamera::SetAspectRatio(float aspectRatio)
{
	m_cameraProperties.aspectRatio = glm::max(aspectRatio, 0.01f);
	UpdateMatrices();
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

void pwg::PlayerCamera::SetTerrainHeightProvider(std::function<float(float, float)> provider)
{
	m_terrainHeightProvider = provider;
}

glm::vec3 pwg::PlayerCamera::GetCameraPosition()
{
	return m_transform.position;
}

void pwg::PlayerCamera::HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt)
{
	glm::vec3 planarForward = glm::normalize(glm::vec3(m_cameraProperties.forward.x, 0.0f, m_cameraProperties.forward.z));
	glm::vec3 cameraRight = glm::normalize(glm::cross(planarForward, m_cameraProperties.up));
	glm::vec3 desiredDirection{ 0.0f };

	if (keyboardInput.IsHeld(Action::MoveForward))
		desiredDirection += planarForward;
	if (keyboardInput.IsHeld(Action::MoveBackward))
		desiredDirection -= planarForward;
	if (keyboardInput.IsHeld(Action::MoveLeft))
		desiredDirection -= cameraRight;
	if (keyboardInput.IsHeld(Action::MoveRight))
		desiredDirection += cameraRight;

	if (glm::dot(desiredDirection, desiredDirection) > 0.0001f)
	{
		desiredDirection = glm::normalize(desiredDirection);
		m_playerCameraProperties.velocity += desiredDirection * m_playerCameraProperties.acceleration * dt;
	}

	glm::vec2 horizontalVelocity{ m_playerCameraProperties.velocity.x, m_playerCameraProperties.velocity.z };
	float horizontalSpeed = glm::length(horizontalVelocity);
	if (horizontalSpeed > m_playerCameraProperties.maxGroundSpeed)
	{
		horizontalVelocity = glm::normalize(horizontalVelocity) * m_playerCameraProperties.maxGroundSpeed;
		m_playerCameraProperties.velocity.x = horizontalVelocity.x;
		m_playerCameraProperties.velocity.z = horizontalVelocity.y;
	}

	float dampingStrength = m_playerCameraProperties.grounded ? m_playerCameraProperties.groundDamping : m_playerCameraProperties.airDamping;
	float damping = std::clamp(1.0f - dampingStrength * dt, 0.0f, 1.0f);
	m_playerCameraProperties.velocity.x *= damping;
	m_playerCameraProperties.velocity.z *= damping;

	float groundHeight = GetGroundHeight();
	float feetY = m_transform.position.y - m_playerCameraProperties.eyeHeight;
	bool closeToGround = feetY <= groundHeight + m_playerCameraProperties.groundSnapDistance;

	if (closeToGround && m_playerCameraProperties.velocity.y <= 0.0f)
	{
		m_playerCameraProperties.grounded = true;
		m_playerCameraProperties.velocity.y = 0.0f;
		m_transform.position.y = groundHeight + m_playerCameraProperties.eyeHeight;
	}
	else
	{
		m_playerCameraProperties.grounded = false;
	}

	if (keyboardInput.IsPressed(Action::MoveUp) && m_playerCameraProperties.grounded)
	{
		m_playerCameraProperties.velocity.y = m_playerCameraProperties.jumpSpeed;
		m_playerCameraProperties.grounded = false;
	}

	m_playerCameraProperties.velocity.y -= m_playerCameraProperties.gravity * dt;
	m_transform.position += m_playerCameraProperties.velocity * dt;

	groundHeight = GetGroundHeight();
	if (m_transform.position.y - m_playerCameraProperties.eyeHeight < groundHeight)
	{
		m_transform.position.y = groundHeight + m_playerCameraProperties.eyeHeight;
		m_playerCameraProperties.velocity.y = 0.0f;
		m_playerCameraProperties.grounded = true;
	}
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
}

void pwg::PlayerCamera::CalculateCameraForward()
{
	float yawRad = glm::radians(m_playerCameraProperties.yaw);
	float pitchRad = glm::radians(m_playerCameraProperties.pitch);

	m_cameraProperties.forward = glm::normalize(glm::vec3(cos(pitchRad) * cos(yawRad), sin(pitchRad), cos(pitchRad) * sin(yawRad)));
}

void pwg::PlayerCamera::UpdateMatrices()
{
	m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_transform.position + m_cameraProperties.forward, m_cameraProperties.up);
	m_cameraProperties.projectionMatrix = glm::perspective(glm::radians(m_cameraProperties.fov), m_cameraProperties.aspectRatio, m_cameraProperties.nearPlane, m_cameraProperties.farPlane);
}

float pwg::PlayerCamera::GetGroundHeight() const
{
	if (!m_terrainHeightProvider)
	{
		return 0.0f;
	}

	return m_terrainHeightProvider(m_transform.position.x, m_transform.position.z);
}
