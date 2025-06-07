#include "camera.h"

pwg::Camera::Camera()
{
	
}

void pwg::Camera::UpdateCamera(const unsigned int& shaderID)
{
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(m_cameraPosition, m_cameraPosition + m_cameraFront, m_cameraUp);

	glUseProgram(shaderID);

	int viewLocation = glGetUniformLocation(shaderID, "view");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);
}

void pwg::Camera::MoveCamera(GLFWwindow* window, pwg::KeyboardInput* input, const float& dt, pwg::MouseInput* minput)
{
	const float cameraSpeed = 0.01f; 
	if (input->IsHeld(Action::MoveForward))
		m_cameraPosition += cameraSpeed * dt *  m_cameraFront;
	if (input->IsHeld(Action::MoveBackward))
		m_cameraPosition -= cameraSpeed * dt * m_cameraFront;
	if (input->IsHeld(Action::MoveLeft))
		m_cameraPosition -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * dt *
		cameraSpeed;
	if (input->IsHeld(Action::MoveRight))
		m_cameraPosition += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * dt *
		cameraSpeed;
}

void pwg::Camera::UpdateRotation(pwg::MouseInput* minput)
{
	float sensitivity = 0.1f;

	float xoffset = minput->GetDeltaX() * sensitivity;
	float yoffset = minput->GetDeltaY() * sensitivity;

	// Aktualizujemy k¹ty yaw i pitch
	m_yaw += xoffset;
	m_pitch += yoffset;

	// Ograniczamy pitch, ¿eby kamera nie obraca³a siê "do góry nogami"
	if (m_pitch > 89.0f) m_pitch = 89.0f;
	if (m_pitch < -89.0f) m_pitch = -89.0f;

	// Przeliczamy wektor front kamery na podstawie yaw i pitch (konwersja z k¹tów do wektora)
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_cameraFront = glm::normalize(front);

}
