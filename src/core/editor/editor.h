#ifndef SRC_CORE_EDITOR_EDITOR_H
#define SRC_CORE_EDITOR_EDITOR_H

#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/gui/gui.h"


namespace pwg
{
	//Enum that stores actions which could be binded to a functional key
	enum class EditorType
	{
		Window = 0,
		Noise,
		Color,
		Controls,
		Count
	};

	//Precompiled size of a enum class
	constexpr int NumEditors = static_cast<int>(pwg::EditorType::Count);

	class Editor
	{
	public:
		Editor(Window& window);
		~Editor();

		void InitEditor();
		void Update();
		void Render();

		void ShowEditor();
		bool IsActive();
	private:
		bool m_isActive{ false };
		pwg::Gui* m_gui;
		pwg::Window m_window;

	};
}
#endif // !SRC_CORE_EDITOR_EDITOR_H
