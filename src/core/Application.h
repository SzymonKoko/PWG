#ifndef APPLICATION_H_
#define APPLICATION_H_


#include <core/window.h>
#include "graphics/mesh.h"


namespace pwg
{
	class Application
	{
	public:
		Application();
		~Application();

		void Run();

	private:
		pwg::Window m_window;
		void InitApplication();
		GLfloat vertices[18] = 
		{
			-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower left corner
			0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower right corner
			0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f, // Upper corner
			-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner left
			0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner right
			0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f // Inner down
		};

		// Indices for vertices order
		GLuint indices[9] =
		{
			0, 3, 5, // Lower left triangle
			3, 2, 4, // Lower right triangle
			5, 4, 1 // Upper triangle
		};
	};
}
#endif // !APPLICATION_H_