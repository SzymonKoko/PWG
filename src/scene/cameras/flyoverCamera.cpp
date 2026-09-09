#include "flyoverCamera.h"
#include <algorithm>
#include <cmath>

namespace pwg
{
	FlyoverCamera::FlyoverCamera()
	{
		m_targetYaw = m_flyoverProperties.yaw;
		m_targetPitch = m_flyoverProperties.pitch;
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
		m_flyoverProperties.velocity = glm::vec3(0.0f);
		m_gimbalRoll = 0.0f;
		UpdateMatrices();
	}

	void FlyoverCamera::SetCameraPosition(float x, float y, float z)
	{
		m_transform.position = { x, y, z };
		m_flyoverProperties.velocity = glm::vec3(0.0f);
		m_gimbalRoll = 0.0f;
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

		bool hasInput = glm::dot(desiredDirection, desiredDirection) > 0.0001f;
		float boost = keyboardInput.IsHeld(Action::MoveBoost) ? m_flyoverProperties.boostMultiplier : 1.0f;
		if (hasInput)
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

		float dampingStrength = hasInput ? m_flyoverProperties.flightDrag : m_flyoverProperties.braking;
		m_flyoverProperties.velocity *= std::exp(-std::max(dampingStrength, 0.0f) * dt);
		m_transform.position += m_flyoverProperties.velocity * dt;

		float lateralSpeed = glm::dot(m_flyoverProperties.velocity, right);
		float speedRatio = lateralSpeed / std::max(m_flyoverProperties.maxSpeed, 1.0f);
		float targetRoll = glm::radians(m_flyoverProperties.gimbalStrengthDegrees)
			* std::clamp(-speedRatio - m_mouseTurn * 0.012f, -1.0f, 1.0f);
		float gimbalResponse = 1.0f - std::exp(-std::max(m_flyoverProperties.gimbalSmoothing, 0.1f) * dt);
		m_gimbalRoll = glm::mix(m_gimbalRoll, targetRoll, gimbalResponse);
		m_mouseTurn = glm::mix(m_mouseTurn, 0.0f, gimbalResponse);
	}

	void FlyoverCamera::HandleMouseInput(MouseInput& mouseInput, float dt)
	{
		float rawDeltaX = mouseInput.GetDeltaX();
		float xoffset = rawDeltaX * m_flyoverProperties.sensitivity;
		float yoffset = mouseInput.GetDeltaY() * m_flyoverProperties.sensitivity;

		m_targetYaw += xoffset;
		m_targetPitch = glm::clamp(m_targetPitch + yoffset, -89.0f, 89.0f);
		float lookResponse = 1.0f - std::exp(-std::max(m_flyoverProperties.lookSmoothing, 0.1f) * dt);
		m_flyoverProperties.yaw = glm::mix(m_flyoverProperties.yaw, m_targetYaw, lookResponse);
		m_flyoverProperties.pitch = glm::mix(m_flyoverProperties.pitch, m_targetPitch, lookResponse);
		m_mouseTurn = glm::mix(m_mouseTurn, rawDeltaX, lookResponse);

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
		glm::vec3 right = glm::normalize(glm::cross(m_cameraProperties.forward, m_cameraProperties.up));
		glm::vec3 cameraUp = glm::normalize(glm::cross(right, m_cameraProperties.forward));
		glm::vec3 gimbalUp = glm::normalize(cameraUp * std::cos(m_gimbalRoll) + right * std::sin(m_gimbalRoll));
		m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_transform.position + m_cameraProperties.forward, gimbalUp);
		m_cameraProperties.projectionMatrix = glm::perspective(glm::radians(m_cameraProperties.fov), m_cameraProperties.aspectRatio, m_cameraProperties.nearPlane, m_cameraProperties.farPlane);
	}
}
