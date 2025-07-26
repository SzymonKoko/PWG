#ifndef SRC_CORE_EDITOR_WINDOW_EDITOR_WINDOW_EDITOR_H
#define SRC_CORE_EDITOR_WINDOW_EDITOR_WINDOW_EDITOR_H

#include "core/logger/logger.h"
#include "core/window/window.h"
#include "core/gui/gui.h"

namespace pwg
{
	class WindowEditor 
	{
	public:
		WindowEditor(Window& window);
		~WindowEditor() = default;

		void InitEditor();
		void Update(const float dt);
		void Render();
		const char* GetName() const;
	private:
		int m_selectedResolution{ 0 };
		Window& m_window;

	};
}
#endif // !SRC_CORE_EDITOR_WINDOW_EDITOR_WINDOW_EDITOR_H

/*
Resolution
Framerate


*/
