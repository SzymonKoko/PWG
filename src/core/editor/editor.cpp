#include "editor.h"

pwg::Editor::Editor(Window& window)
	:m_window(window)
{
}

pwg::Editor::~Editor()
{
}

void pwg::Editor::InitEditor()
{
	m_gui = new Gui(m_window);
	m_gui->InitGui();
}

void pwg::Editor::Update()
{
	m_gui->Update();
}

void pwg::Editor::Render()
{
	m_gui->Render();
}

void pwg::Editor::ShowEditor()
{
}

bool pwg::Editor::IsActive()
{
	return m_isActive;
}
