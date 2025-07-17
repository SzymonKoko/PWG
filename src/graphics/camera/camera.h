#ifndef SRC_GRAPHICS_CAMERA_CAMERA_H_
#define SRC_GRAPHICS_CAMERA_CAMERA_H_

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"

namespace pwg
{
	class Camera
	{
	public:
		Camera();
		~Camera() = default;

		void UpdateCamera(GLFWwindow* window, KeyboardInput* input, const float& dt, pwg::MouseInput* minput, const unsigned int& shaderID);
	private:
		glm::vec3 m_cameraPosition{ -100.0f, 50.0f, 30.0f };
		glm::vec3 m_cameraTarget{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_cameraUp{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_cameraFront{ 0.0f, 0.0f, -1.0f };
		float m_yaw{ -90.0f };
		float m_pitch{ 0.0f };
	};
}
#endif // !SRC_GRAPHICS_CAMERA_CAMERA_H_
