#include <glad/glad.h>
#include "imgui.h"
#include "window_editor.h"

pwg::WindowEditor::WindowEditor(Window& window)
	:m_window(window)
{
}

void pwg::WindowEditor::InitEditor()
{
	Logger::LogInfo(Logger::Module::WindowEditor, "WindowEditor init");
}

void pwg::WindowEditor::Update(const float dt)
{
	Logger::LogInfo(Logger::Module::WindowEditor, "WindowEditor update");
}

void pwg::WindowEditor::Render()
{
	//Logger::LogInfo(Logger::Module::WindowEditor, "WindowEditor render");

	const char* resolutions[] = { "800x600", "1280x720", "1366x768", "1920x1080"};
	const glm::vec2 resolutionSizes[] = {
		{800, 600},
		{1280, 720},
		{1366, 768},
		{1920, 1080}
	};

	ImGui::SetNextWindowSize(ImVec2(400, 200));
	ImGui::Begin("Window Editor");

	
	if (ImGui::Combo("Resolutions", &m_selectedResolution, resolutions, IM_ARRAYSIZE(resolutions)))
	{
		glm::vec2 newSize = resolutionSizes[m_selectedResolution];
		m_window.SetWindowSizeAndPosition(m_window.GetWindow(), newSize.x, newSize.y);
	}
	
	ImGui::End();
}

const char* pwg::WindowEditor::GetName() const
{
	return "Window Editor";
}
