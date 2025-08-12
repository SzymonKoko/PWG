#include <glad/glad.h>
#include "imgui.h"
#include "scene.h"

pwg::Scene::Scene(GLFWwindow* window, MouseInput* minput, KeyboardInput* kinput)
    :m_keyboardInput(kinput), m_mouseInput(minput), m_window(window)
{
    m_frameBuffer = std::make_unique<FrameBuffer>(800, 800, true);
    m_editorCamera = std::make_unique<EditorCamera>(m_window, m_mouseInput, m_keyboardInput);
}

void pwg::Scene::Update(const float& dt)
{
    m_editorCamera->Update();

    float aspectRatio = static_cast<float>(m_frameBuffer->GetWidth()) / static_cast<float>(m_frameBuffer->GetHeight());
    m_editorCamera->UpdateProjectionMatrix(aspectRatio);

    m_renderer.SetCamera(m_editorCamera.get());
}

void pwg::Scene::Draw()
{
    ImGui::Begin("Scene");

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("SceneRenderArea", windowSize, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    int width = static_cast<int>(windowSize.x);
    int height = static_cast<int>(windowSize.y);

    if (m_frameBuffer->GetWidth() != width || m_frameBuffer->GetHeight() != height) 
    {
        m_frameBuffer->Resize(width, height);
        
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

