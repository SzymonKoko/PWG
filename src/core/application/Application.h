#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_


#include "core/window/window.h"
#include "graphics/renderer/renderer.h"
#include "graphics/mesh/triangleMesh.h"
#include "graphics/mesh/pyramidMesh.h"
#include "graphics/shader/shader.h"

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
		pwg::PyramidMesh m_pyramidMesh;
		void InitApplication();
		
	};
} //namespace pwg
#endif // !SRC_CORE_APPLICATION_APPLICATION_H_