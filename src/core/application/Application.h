#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_


#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/logger/logger.h"
#include "graphics/renderer/renderer.h"
#include "graphics/mesh/triangleMesh.h"
#include "graphics/mesh/pyramidMesh.h"
#include "graphics/shader/shader.h"
#include "graphics/camera/camera.h"

namespace pwg
{
	class Application
	{
	public:
		Application();
		~Application();

		void Update();
		void Render();
		void Run();

	private:
		pwg::Window m_window;
		pwg::Renderer m_renderer;
		pwg::Shader* shaderProgram;
		pwg::TriangleMesh m_triangleMesh;
		pwg::PyramidMesh m_pyramidMesh;
		pwg::Camera m_camera;
		pwg::KeyboardInput* m_keyboardInput;
		pwg::MouseInput* m_mouseInput;
		void InitApplication();
		
	};
} //namespace pwg
#endif // !SRC_CORE_APPLICATION_APPLICATION_H_