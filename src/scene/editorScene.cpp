#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"

pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput* minput, KeyboardInput* kinput)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(800, 800, true);
}

pwg::EditorScene::EditorScene(const EditorScene& otherEditorScene)
    : m_window(otherEditorScene.m_window),
      m_keyboardInput(otherEditorScene.m_keyboardInput),
      m_mouseInput(otherEditorScene.m_mouseInput), 
      m_camera(otherEditorScene.m_camera),
      m_renderer(otherEditorScene.m_renderer)
{
    if (otherEditorScene.m_frameBuffer)
    {
        m_frameBuffer = std::make_unique<FrameBuffer>(*otherEditorScene.m_frameBuffer);
    }
}

pwg::EditorScene& pwg::EditorScene::operator=(const EditorScene& otherEditorScene)
{
    if (this == &otherEditorScene)
    {
        return *this;
    }

    m_window = otherEditorScene.m_window;
    m_keyboardInput = otherEditorScene.m_keyboardInput;
    m_mouseInput = otherEditorScene.m_mouseInput;
    m_camera = otherEditorScene.m_camera;
    m_renderer = otherEditorScene.m_renderer;

    if (otherEditorScene.m_frameBuffer)
    {
        m_frameBuffer = std::make_unique<FrameBuffer>(*otherEditorScene.m_frameBuffer);
    }
    else
    {
        m_frameBuffer.reset();
    }

    return *this;
}

void pwg::EditorScene::Update(const float& dt)
{
	m_camera.UpdateCamera(m_window, m_keyboardInput, dt, m_mouseInput, m_renderer.GetShaderProgramID());
}

void pwg::EditorScene::Draw()
{
    ImGui::Begin("Scene");

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("SceneRenderArea", windowSize, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    int width = static_cast<int>(windowSize.x);
    int height = static_cast<int>(windowSize.y);

    if (m_frameBuffer->GetWidth() != width || m_frameBuffer->GetHeight() != height)
    {
        m_frameBuffer->Resize(width, height);
        m_frameBuffer->CheckResize();
    }

    m_frameBuffer->Bind();

    m_renderer.Clear();
    m_renderer.Update();
    m_renderer.Draw();

    m_frameBuffer->Unbind();

    ImGui::Image(
        (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
        windowSize,
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::EndChild();

    ImGui::End();
}

std::unique_ptr<pwg::IScene> pwg::EditorScene::Clone()
{
	return std::make_unique<pwg::EditorScene>(*this);
}
