#ifndef SRC_CORE_WINDOW_WINDOW_H_
#define SRC_CORE_WINDOW_WINDOW_H_

#include <iostream>
#include <GLFW/glfw3.h>
#include "core/logger/logger.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"

namespace pwg
{
	class Window
	{
	public:

		/**
		* @brief Window constructor. Initializing GLFW, Glad and default configs
		* @param minput Reference to a mouse input instance that is used for window
		* @param kinput Reference to a keyboard input instance that is used for window
		*/
		Window(MouseInput& mouseInput, KeyboardInput& keyboardInput);

		/**
		* @brief Window destructor. Destroying GLFWwindow instance.
		*/
		~Window();


		/**
		* @brief Updates window on every frame.
		* @param dt Time between frames
		*/
		void Update(float dt);

		/**
		* @brief Swaps buffer. GLFW standard function.
		*/
		void SwapBuffers();

		/**
		* @brief Changes state of a window. Waits for user input
		* @return Returns true if window should close, false otherwise
		*/
		bool WindowShouldClose() const;

		/**
		* @brief GLFWwindow getter.
		* @return Returns GLFWwindow instance
		*/
		GLFWwindow* GetWindow() { return m_window; }

		/**
		* @brief Delta time getter
		* @return Returns time between frames
		*/
		float GetDeltaTime() { return m_deltaTime; }

		/**
		* @brief Window width getter
		* @return Returns width of the window in pixels
		*/
		float GetWindowWidth() { return m_windowWidth; }

		/**
		* @brief Window height getter
		* @return Returns height of the window in pixels
		*/
		float GetWindowHeight() { return m_windowHeight; }

		/**
		* @brief Window size setter
		* @param width New width of the window in pixels
		* @param height New height of the window in pixels
		*/
		void SetWindowSize(float width, float height);

		
	private:
		GLFWwindow* m_window;
		MouseInput& m_mouseInput;
		KeyboardInput& m_keyboardInput;

		uint16_t m_windowWidth{ 1600 };
		uint16_t m_windowHeight{ 900 };
		uint16_t m_screenWidth{ 0 };
		uint16_t m_screenHeight{ 0 };
		float m_deltaTime{ 0.0f };
		float m_lastFrame{ 0.0f };

		/**
		* @brief Polls GLFW events. GLFW standard function.
		*/
		void PollEvents();

		/**
		* @brief Updates delta time. Calculates time between frames
		*/
		void UpdateDeltaTime();

		/**
		* @brief GLFW scroll callback function. Callback triggered when scroll is moved
		* @param window Pointer to GLFWwindow instance
		* @param xoffset Scroll offset on X axis
		* @param yoffset Scroll offset on y axis
		*/
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

		/**
		* @brief Framebuffer size change callback function. Callback triggered when framebuffer size changes.
		* @param window Pointer to GLFWwindow instance
		* @param width New width of a framebuffer
		* @param height New height of a framebuffer
		*/
		static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	};
} //namespace pwg

#endif // !SRC_CORE_WINDOW_WINDOW_H_
