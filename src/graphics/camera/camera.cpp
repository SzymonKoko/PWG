#include "camera.h"

pwg::Camera::Camera()
{
	
}

void pwg::Camera::UpdateCamera(GLFWwindow* window, pwg::KeyboardInput* input, const float& dt, pwg::MouseInput* minput, const unsigned int& shaderID)
{
	//Creating view matrix for a camera
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(m_cameraPosition, m_cameraPosition + m_cameraFront, m_cameraUp);

	glUseProgram(shaderID);

	int viewLocation = glGetUniformLocation(shaderID, "view");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	//Moving camera with keyboard
	const float cameraSpeed = 0.001f; 
	if (input->IsHeld(Action::MoveForward))
		m_cameraPosition += cameraSpeed * dt *  m_cameraFront;
	if (input->IsHeld(Action::MoveBackward))
		m_cameraPosition -= cameraSpeed * dt * m_cameraFront;
	if (input->IsHeld(Action::MoveLeft))
		m_cameraPosition -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * dt * cameraSpeed;
	if (input->IsHeld(Action::MoveRight))
		m_cameraPosition += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * dt * cameraSpeed;

	//Moving camera up and down
	if (input->IsHeld(Action::MoveUp))
		m_cameraPosition += cameraSpeed * dt * m_cameraUp;
	if (input->IsHeld(Action::MoveDown))
		m_cameraPosition -= cameraSpeed * dt * m_cameraUp;

	//Moving camera with mouse
	float sensitivity = 0.1f;

	float xoffset = minput->GetDeltaX() * sensitivity;
	float yoffset = minput->GetDeltaY() * sensitivity;

	//Updating yaw and pitch angles
	m_yaw += xoffset;
	m_pitch += yoffset;

	//Limit pitch to avoid upside down camera moving
	if (m_pitch > 89.0f)
	{
		m_pitch = 89.0f;
	}
	if (m_pitch < -89.0f)
	{
		m_pitch = -89.0f;
	}

	//Converting camera front angles to vectors
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

	//Updating camera
	m_cameraFront = glm::normalize(front);
}
