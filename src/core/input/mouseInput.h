#ifndef SRC_CORE_INPUT_MOUSE_INPUT_H_
#define SRC_CORE_INPUT_MOUSE_INPUT_H_


#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace pwg
{

	class MouseInput
	{
	public:

		MouseInput(GLFWwindow* window);
		~MouseInput() = default;

		void Update();
		bool IsButtonDown(int button) const;

		glm::vec3 GetDirection() const { return m_direction; };
		float GetDeltaX() const { return m_deltaX; }
		float GetDeltaY() const { return m_deltaY; }

	private:
		GLFWwindow* m_window = nullptr;
		glm::vec3 m_direction{ 0.0f, 0.0f, 0.0f };
		bool m_firstClick{ true };
		double m_lastX{ 0.0f };
		double m_lastY{ 0.0f };

		float m_deltaX{ 0.0f };
		float m_deltaY{ 0.0f };
	};
}

#endif // !SRC_CORE_INPUT_MOUSE_INPUT_H_
