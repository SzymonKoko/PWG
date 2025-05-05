#ifndef APPLICATION_H_
#define APPLICATION_H_


#include "window/window.h"
#include "renderer/renderer.h"
#include "graphics/mesh/triangleMesh.h"
#include "graphics/shader.h"




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
		pwg::Renderer m_renderer;
		pwg::Shader* shaderProgram;
		pwg::TriangleMesh m_triangleMesh;
		void InitApplication();
		
	};
}
#endif // !APPLICATION_H_