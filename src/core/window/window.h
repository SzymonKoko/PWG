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

		void Update();
		void SwapBuffers();
		bool WindowShouldClose(KeyboardInput* input) const;
		

		GLFWwindow* GetWindow() { return m_window; }
		float GetDeltaTime();
		
	private:
		GLFWwindow* m_window;
		uint16_t m_windowWidth{ 800 };
		uint16_t m_windowHeight{ 800 };
		float deltaTime{ 0.0f };

		void InitWindow();
		void PollEvents();
		void UpdateDeltaTime();

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height); //Allows to resize the window
	};
} //namespace pwg

#endif // !SRC_CORE_WINDOW_WINDOW_H_
