#ifndef SRC_CORE_WINDOW_WINDOW_H_
#define SRC_CORE_WINDOW_WINDOW_H_


#include <iostream>
#include <glad.h>
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

		
		bool WindowShouldClose(KeyboardInput* input) const;
		void PollEvents();
		void SwapBuffers();

		void UpdateDeltaTime();

		GLFWwindow* GetWindow() { return m_window; }
		float GetDeltaTime();
		
	private:
		GLFWwindow* m_window;
		uint16_t m_windowWidth{ 800 };
		uint16_t m_windowHeight{ 800 };
		float deltaTime{ 0.0f };

		void InitWindow();

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height); //Allows to resize the window
	};
} //namespace pwg

#endif // !SRC_CORE_WINDOW_WINDOW_H_
