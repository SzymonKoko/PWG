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
		Window();
		~Window();

		void Update();
		void SwapBuffers();
		bool WindowShouldClose(KeyboardInput* input) const;

		void RegisterMouseInput(pwg::MouseInput* mouseInput);

		GLFWwindow* GetWindow() { return m_window; }
		float GetDeltaTime();
		float GetWindowWidth();
		float GetWindowHeight();

		void SetWindowSizeAndPosition(GLFWwindow* window, int newWidth, int newHeight);
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	private:
		GLFWwindow* m_window;

		uint16_t m_windowWidth{ 1200 };
		uint16_t m_windowHeight{ 1200 };
		uint16_t m_screenWidth{ 0 };
		uint16_t m_screenHeight{ 0 };
		float deltaTime{ 0.0f };
		float lastFrame{ 0.0f };

		void InitWindow();
		void PollEvents();
		void UpdateDeltaTime();

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height); //Allows to resize the window
		
		static pwg::MouseInput* s_mouseInput;
	};
} //namespace pwg

#endif // !SRC_CORE_WINDOW_WINDOW_H_
