#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_


#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/logger/logger.h"
#include "graphics/renderer/renderer.h"
#include "graphics/mesh/triangleMesh.h"
#include "graphics/shader/shader.h"
#include "graphics/camera/camera.h"
#include "core/gui/gui.h"
#include "core/editor/window_editor/window_editor.h"

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
		pwg::PyramidMesh m_pyramidMesh;
		pwg::Camera m_camera;
		pwg::KeyboardInput* m_keyboardInput;
		pwg::MouseInput* m_mouseInput;
		pwg::Gui* m_gui;
		pwg::WindowEditor* m_windowEditor;
		void InitApplication();
		
	};
} //namespace pwg
#endif // !SRC_CORE_APPLICATION_APPLICATION_H_