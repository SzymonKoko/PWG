#ifndef SRC_CORE_EDITOR_WINDOW_EDITOR_WINDOW_EDITOR_H
#define SRC_CORE_EDITOR_WINDOW_EDITOR_WINDOW_EDITOR_H

#include "core/editor/ieditor.h"
#include "core/logger/logger.h"
#include "core/window/window.h"
#include "core/gui/gui.h"

namespace pwg
{
	class WindowEditor : IEditor
	{
	public:
		WindowEditor(Window& window);
		~WindowEditor() = default;

		void InitEditor() override;
		void Update(const float dt) override;
		void Render() override;
		const char* GetName() const override;
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
