#include "playerCamera.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

pwg::PlayerCamera::PlayerCamera()
{
	CalculateCameraForward();
	ResetCameraMotion();
	UpdateMatrices();
}

void pwg::PlayerCamera::Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
{
	dt = std::clamp(dt, 0.0f, 0.05f);
	bool wasGrounded = m_playerCameraProperties.grounded;
	float previousVerticalVelocity = m_playerCameraProperties.velocity.y;
	HandleMouseInput(mouseInput, dt);
	HandleKeyboardInput(keyboardInput, dt);
	UpdateCameraMotion(dt, wasGrounded, previousVerticalVelocity);
	UpdateMatrices();
}

void pwg::PlayerCamera::SetDefaultCameraPosition(int size)
{
	m_transform.position = { 0.0f, 500.0f, 0.0f };
	m_playerCameraProperties.velocity = { 0.0f, 0.0f, 0.0f };
	ResetCameraMotion();
	UpdateMatrices();
}

void pwg::PlayerCamera::SetCameraPosition(float x, float y, float z)
{
	m_transform.position = { x, y, z };
	m_playerCameraProperties.velocity = { 0.0f, 0.0f, 0.0f };
	ResetCameraMotion();
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
	return m_viewPosition;
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

void pwg::PlayerCamera::UpdateCameraMotion(float dt, bool wasGrounded, float previousVerticalVelocity)
{
	float response = 1.0f - std::exp(-m_playerCameraProperties.headBobResponse * dt);
	glm::vec2 horizontalVelocity(m_playerCameraProperties.velocity.x, m_playerCameraProperties.velocity.z);
	float horizontalSpeed = glm::length(horizontalVelocity);
	float speedWeight = glm::smoothstep(0.35f, 8.0f, horizontalSpeed);
	float targetWeight = m_playerCameraProperties.grounded && m_playerCameraProperties.headBobEnabled ? speedWeight : 0.0f;
	m_headBobWeight = glm::mix(m_headBobWeight, targetWeight, response);

	if (m_playerCameraProperties.grounded && horizontalSpeed > 0.2f)
	{
		float strideLength = std::max(m_playerCameraProperties.headBobStrideLength, 0.5f);
		m_headBobPhase = std::fmod(
			m_headBobPhase + horizontalSpeed * dt * glm::two_pi<float>() / strideLength,
			glm::two_pi<float>());
	}

	float targetSide = std::sin(m_headBobPhase) * m_playerCameraProperties.headBobSide * m_headBobWeight;
	float targetHeight = std::sin(m_headBobPhase * 2.0f) * m_playerCameraProperties.headBobHeight * m_headBobWeight;
	float targetRoll = -std::sin(m_headBobPhase) * glm::radians(m_playerCameraProperties.headBobRollDegrees) * m_headBobWeight;
	m_headBobSideOffset = glm::mix(m_headBobSideOffset, targetSide, response);
	m_headBobHeightOffset = glm::mix(m_headBobHeightOffset, targetHeight, response);
	m_headBobRoll = glm::mix(m_headBobRoll, targetRoll, response);

	if (!wasGrounded && m_playerCameraProperties.grounded && previousVerticalVelocity < -2.0f)
	{
		float impact = std::clamp(-previousVerticalVelocity * 0.012f, 0.04f, 0.32f);
		m_landingOffset -= impact;
	}
	constexpr float landingSpring = 95.0f;
	constexpr float landingDamping = 18.0f;
	m_landingVelocity += (-landingSpring * m_landingOffset - landingDamping * m_landingVelocity) * dt;
	m_landingOffset += m_landingVelocity * dt;

	float verticalResponse = 1.0f - std::exp(-(m_playerCameraProperties.grounded ? 18.0f : 30.0f) * dt);
	m_smoothedEyeY = glm::mix(m_smoothedEyeY, m_transform.position.y, verticalResponse);
	float maxVerticalLag = m_playerCameraProperties.eyeHeight * 0.3f;
	m_smoothedEyeY = glm::clamp(
		m_smoothedEyeY,
		m_transform.position.y - maxVerticalLag,
		m_transform.position.y + maxVerticalLag);

	glm::vec3 planarForward(m_cameraProperties.forward.x, 0.0f, m_cameraProperties.forward.z);
	planarForward = glm::dot(planarForward, planarForward) < 0.0001f
		? glm::vec3(0.0f, 0.0f, -1.0f)
		: glm::normalize(planarForward);
	glm::vec3 right = glm::normalize(glm::cross(planarForward, m_cameraProperties.up));
	m_viewPosition = glm::vec3(m_transform.position.x, m_smoothedEyeY, m_transform.position.z)
		+ right * m_headBobSideOffset
		+ m_cameraProperties.up * (m_headBobHeightOffset + m_landingOffset);
}

void pwg::PlayerCamera::ResetCameraMotion()
{
	m_viewPosition = m_transform.position;
	m_smoothedEyeY = m_transform.position.y;
	m_headBobPhase = 0.0f;
	m_headBobWeight = 0.0f;
	m_headBobSideOffset = 0.0f;
	m_headBobHeightOffset = 0.0f;
	m_headBobRoll = 0.0f;
	m_landingOffset = 0.0f;
	m_landingVelocity = 0.0f;
}

void pwg::PlayerCamera::UpdateMatrices()
{
	glm::vec3 right = glm::normalize(glm::cross(m_cameraProperties.forward, m_cameraProperties.up));
	glm::vec3 cameraUp = glm::normalize(glm::cross(right, m_cameraProperties.forward));
	glm::vec3 rolledUp = glm::normalize(cameraUp * std::cos(m_headBobRoll) + right * std::sin(m_headBobRoll));
	m_cameraProperties.viewMatrix = glm::lookAt(m_viewPosition, m_viewPosition + m_cameraProperties.forward, rolledUp);
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
