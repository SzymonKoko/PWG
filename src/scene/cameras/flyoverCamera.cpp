#include "flyoverCamera.h"
#include <algorithm>

namespace pwg
{
	FlyoverCamera::FlyoverCamera()
	{
		CalculateCameraForward();
		UpdateMatrices();
	}

	void FlyoverCamera::Update(float dt, MouseInput& mouseInput, KeyboardInput& keyboardInput)
	{
		dt = std::clamp(dt, 0.0f, 0.05f);
		HandleMouseInput(mouseInput, dt);
		HandleKeyboardInput(keyboardInput, dt);
		UpdateMatrices();
	}

	void FlyoverCamera::SetDefaultCameraPosition(int size)
	{
		float halfSize = static_cast<float>(size) * 0.5f;
		m_transform.position = { halfSize * 0.45f, halfSize * 0.35f, -halfSize * 0.45f };
		UpdateMatrices();
	}

	void FlyoverCamera::SetCameraPosition(float x, float y, float z)
	{
		m_transform.position = { x, y, z };
		UpdateMatrices();
	}

		void FlyoverCamera::SetAspectRatio(float aspectRatio)
	{
		m_cameraProperties.aspectRatio = glm::max(aspectRatio, 0.01f);
		UpdateMatrices();
	}
	glm::mat4& FlyoverCamera::GetViewMatrix()
	{
		return m_cameraProperties.viewMatrix;
	}

	glm::mat4& FlyoverCamera::GetProjectionMatrix()
	{
		return m_cameraProperties.projectionMatrix;
	}

	glm::vec3 FlyoverCamera::GetCameraPosition()
	{
		return m_transform.position;
	}

	void FlyoverCamera::HandleKeyboardInput(KeyboardInput& keyboardInput, float dt)
	{
		glm::vec3 desiredDirection{ 0.0f };
		glm::vec3 right = glm::normalize(glm::cross(m_cameraProperties.forward, m_cameraProperties.up));

		if (keyboardInput.IsHeld(Action::MoveForward))
			desiredDirection += m_cameraProperties.forward;
		if (keyboardInput.IsHeld(Action::MoveBackward))
			desiredDirection -= m_cameraProperties.forward;
		if (keyboardInput.IsHeld(Action::MoveLeft))
			desiredDirection -= right;
		if (keyboardInput.IsHeld(Action::MoveRight))
			desiredDirection += right;
		if (keyboardInput.IsHeld(Action::MoveUp))
			desiredDirection += m_cameraProperties.up;
		if (keyboardInput.IsHeld(Action::MoveDown))
			desiredDirection -= m_cameraProperties.up;

		float boost = keyboardInput.IsHeld(Action::MoveBoost) ? m_flyoverProperties.boostMultiplier : 1.0f;
		if (glm::dot(desiredDirection, desiredDirection) > 0.0001f)
		{
			desiredDirection = glm::normalize(desiredDirection);
			m_flyoverProperties.velocity += desiredDirection * m_flyoverProperties.acceleration * boost * dt;
		}

		float maxSpeed = m_flyoverProperties.maxSpeed * boost;
		float speed = glm::length(m_flyoverProperties.velocity);
		if (speed > maxSpeed)
		{
			m_flyoverProperties.velocity = glm::normalize(m_flyoverProperties.velocity) * maxSpeed;
		}

		float damping = std::clamp(1.0f - m_flyoverProperties.damping * dt, 0.0f, 1.0f);
		m_flyoverProperties.velocity *= damping;
		m_transform.position += m_flyoverProperties.velocity * dt;
	}

	void FlyoverCamera::HandleMouseInput(MouseInput& mouseInput, float dt)
	{
		float xoffset = mouseInput.GetDeltaX() * m_flyoverProperties.sensitivity;
		float yoffset = mouseInput.GetDeltaY() * m_flyoverProperties.sensitivity;

		m_flyoverProperties.yaw += xoffset;
		m_flyoverProperties.pitch += yoffset;
		m_flyoverProperties.pitch = glm::clamp(m_flyoverProperties.pitch, -89.0f, 89.0f);

		CalculateCameraForward();
	}

	void FlyoverCamera::CalculateCameraForward()
	{
		float yawRad = glm::radians(m_flyoverProperties.yaw);
		float pitchRad = glm::radians(m_flyoverProperties.pitch);
		m_cameraProperties.forward = glm::normalize(glm::vec3(cos(pitchRad) * cos(yawRad), sin(pitchRad), cos(pitchRad) * sin(yawRad)));
	}

	void FlyoverCamera::UpdateMatrices()
	{
		m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_transform.position + m_cameraProperties.forward, m_cameraProperties.up);
		m_cameraProperties.projectionMatrix = glm::perspective(glm::radians(m_cameraProperties.fov), m_cameraProperties.aspectRatio, m_cameraProperties.nearPlane, m_cameraProperties.farPlane);
	}
}
