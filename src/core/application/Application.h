#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_

#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/logger/logger.h"
#include "core/gui/gui.h"
#include "core/editor/window_editor/window_editor.h"
#include "graphics/scene/scene.h"

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
		//TODO more smart pointers
		pwg::Window m_window;
		pwg::KeyboardInput* m_keyboardInput; //TODO InputManager class
		pwg::MouseInput* m_mouseInput; //TODO InputManager class
		std::unique_ptr<Gui> m_gui;
		pwg::WindowEditor* m_windowEditor; // TODO Move to GUI
		std::shared_ptr<Scene> m_scene;
		
	};
} //namespace pwg
#endif // !SRC_CORE_APPLICATION_APPLICATION_H_