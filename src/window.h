#ifndef WINDOW_H_
#define WINDOW_H_


#include <iostream>
#include <glad.h>
#include <GLFW/glfw3.h>


namespace pwg 
{
	class Window
	{
	public:
		Window();
		~Window();

		
		bool WindowShouldClose() const;
		void PollEvents();
		void SwapBuffers();
		
	private:
		GLFWwindow* m_window;

		void InitWindow();

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height); //Allows to resize the window
	};
}

#endif // !WINDOW_H_
