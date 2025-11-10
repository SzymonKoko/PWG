#ifndef SRC_CORE_WINDOW_WINDOW_H_
#define SRC_CORE_WINDOW_WINDOW_H_

#include <iostream>
#include <GLFW/glfw3.h>
#include "core/logger/logger.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include <chrono>
#include <thread>

namespace pwg
{
	/**
	* @brief Class responsible for creating and managing the application window.
	* Handles GLFW initialization, delta time calculation, and window events.
	*/
	class Window
	{
	public:

		/**
		* @brief Constructs the Window instance.
		* Initializes GLFW, sets up input callbacks, and creates a window with default configuration.
		* @param mouseInput Reference to a MouseInput instance used for handling mouse events.
		* @param keyboardInput Reference to a KeyboardInput instance used for handling keyboard input.
		*/
		Window(MouseInput& mouseInput, KeyboardInput& keyboardInput);

		/**
		* @brief Destructor. Cleans up GLFW and destroys the window instance.
		*/
		~Window();

		/**
		* @brief Updates window state every frame.
		* Polls events and updates delta time.
		* @param dt Time elapsed since last frame.
		*/
		void Update(float dt);

		/**
		* @brief Swaps the front and back buffers.
		* Standard GLFW function used for frame presentation.
		*/
		void SwapBuffers();

		/**
		* @brief Checks if the window should close.
		* @return True if user requested to close the window, false otherwise.
		*/
		bool WindowShouldClose() const;

		/**
		* @brief Getter for the GLFW window pointer.
		* @return Pointer to the GLFWwindow instance.
		*/
		GLFWwindow* GetWindow() { return m_window; }

		/**
		* @brief Getter for delta time value.
		* @return Time elapsed between frames.
		*/
		float GetDeltaTime() { return m_deltaTime; }

		/**
		* @brief Getter for window width in pixels.
		* @return Width of the window.
		*/
		float GetWindowWidth() { return m_windowWidth; }

		/**
		* @brief Getter for window height in pixels.
		* @return Height of the window.
		*/
		float GetWindowHeight() { return m_windowHeight; }

		float GetFrameTimeLimit() { return m_frameTimeLimit; }

		/**
		* @brief Sets a new window size.
		* @param width New width in pixels.
		* @param height New height in pixels.
		*/
		void SetWindowSize(float width, float height);

		void SetFramerateLimit(unsigned int limit);

		void EnableVSync(bool enabled);

	private:
		GLFWwindow* m_window;				/**< Pointer to the GLFW window instance. */
		MouseInput& m_mouseInput;			/**< Reference to the mouse input handler. */
		KeyboardInput& m_keyboardInput;		/**< Reference to the keyboard input handler. */

		uint16_t m_windowWidth{ 1600 };		/**< Current window width in pixels. */
		uint16_t m_windowHeight{ 900 };		/**< Current window height in pixels. */
		uint16_t m_screenWidth{ 0 };		/**< Current screen width in pixels. */
		uint16_t m_screenHeight{ 0 };		/**< Current screen height in pixels. */
		float m_deltaTime{ 0.0f };			/**< Time elapsed between the last and current frame. */
		float m_lastFrame{ 0.0f };			/**< Timestamp of the last rendered frame. */
		float m_frameTimeLimit{ 0.0f };

		/**
		* @brief Polls GLFW window events such as input and resize.
		*/
		void PollEvents();

		/**
		* @brief Updates the delta time value based on current and previous frame timestamps.
		*/
		void UpdateDeltaTime();

		/**
		* @brief Counts and logs FPS to the console or debug overlay.
		* @param dt Delta time from the current frame.
		*/
		void CountFPS(const float& dt);

		/**
		* @brief GLFW scroll callback function.
		* Triggered when the user scrolls using the mouse wheel.
		* @param window Pointer to GLFWwindow instance.
		* @param xoffset Scroll offset along the X axis.
		* @param yoffset Scroll offset along the Y axis.
		*/
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

		/**
		* @brief GLFW framebuffer resize callback.
		* Triggered when the framebuffer size changes (e.g. window resize).
		* @param window Pointer to GLFWwindow instance.
		* @param width New framebuffer width.
		* @param height New framebuffer height.
		*/
		static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	};
} // namespace pwg

#endif // !SRC_CORE_WINDOW_WINDOW_H_
