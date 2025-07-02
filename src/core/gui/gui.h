#ifndef SRC_CORE_GUI_GUI_H
#define SRC_CORE_GUI_GUI_H

#include "core/window/window.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


namespace pwg
{
	class Gui
	{
	public:
		Gui(Window& window);
		~Gui();

		void InitGui();
		void Update();
		void Render();
	private:
		pwg::Window m_window;
	};
}

#endif // !SRC_CORE_GUI_GUI_H