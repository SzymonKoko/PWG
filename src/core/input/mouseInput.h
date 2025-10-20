#ifndef SRC_CORE_INPUT_MOUSE_INPUT_H_
#define SRC_CORE_INPUT_MOUSE_INPUT_H_

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <core/logger/logger.h>

namespace pwg
{
	/**
	* @brief Handles mouse input events such as movement, clicks, and scrolling.
	*/
	class MouseInput
	{
	public:

		/**
		* @brief Mouse input constructor.
		* @param window Pointer to the GLFW window used for input context.
		*/
		MouseInput(GLFWwindow* window);

		/**
		* @brief Default destructor.
		*/
		~MouseInput() = default;

		/**
		* @brief Updates the current mouse state.
		* Calculates cursor delta and direction between frames.
		*/
		void Update();

		/**
		* @brief Checks if a mouse button is currently pressed.
		* @param button GLFW mouse button code (e.g., GLFW_MOUSE_BUTTON_LEFT).
		* @return True if the button is pressed, false otherwise.
		*/
		bool IsButtonDown(int button) const;

		/**
		* @brief Gets the normalized direction vector based on mouse movement.
		* @return 3D vector representing mouse movement direction.
		*/
		glm::vec3 GetDirection() const { return m_direction; };

		/**
		* @brief Gets the horizontal delta of the mouse movement between frames.
		* @return Float value representing X-axis delta.
		*/
		float GetDeltaX() const { return m_deltaX; }

		/**
		* @brief Gets the vertical delta of the mouse movement between frames.
		* @return Float value representing Y-axis delta.
		*/
		float GetDeltaY() const { return m_deltaY; }

		/**
		* @brief Sets the current scroll offset values.
		* Called from the GLFW scroll callback.
		* @param xoffset Horizontal scroll offset.
		* @param yoffset Vertical scroll offset.
		*/
		void SetScrollOffset(double xoffset, double yoffset);

		/**
		* @brief Returns the vertical scroll offset value.
		* @return Double value representing scroll offset on Y-axis.
		*/
		double GetScrollOffsetY();

		/**
		* @brief Resets stored scroll offset values after handling scroll input.
		*/
		void ResetScrollOffset();

		/**
		* @brief Updates the GLFW window reference used for mouse input.
		* @param window Pointer to the new GLFW window instance.
		*/
		void SetWindow(GLFWwindow* window);

	private:
		GLFWwindow* m_window = nullptr;				/**< Pointer to the GLFW window context */
		glm::vec3 m_direction{ 0.0f, 0.0f, 0.0f };	/**< Current direction vector of mouse movement */
		bool m_firstClick{ true };					/**< Flag used to initialize first mouse position */
		double m_lastX{ 0.0f };						/**< Last recorded mouse X position */
		double m_lastY{ 0.0f };						/**< Last recorded mouse Y position */

		float m_deltaX{ 0.0f };						/**< Difference in X position between frames */
		float m_deltaY{ 0.0f };						/**< Difference in Y position between frames */

		double m_scrollOffsetX{ 0.0f };				/**< Scroll offset on the X-axis */
		double m_scrollOffsetY{ 0.0f };				/**< Scroll offset on the Y-axis */
	};
}

#endif // !SRC_CORE_INPUT_MOUSE_INPUT_H_
