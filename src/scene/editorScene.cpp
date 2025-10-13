#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "scene/systems/editorCameraControllerSystem.h"
#include "scene/components/tagComponent.h"
#include "scene/systems/planeMeshSystem.h"


pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput),
      m_resourceManager(resourceManager),
      m_renderer(renderer)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(800, 800, true);

    Entity editorCam(&m_editorSceneRegistry, "EditorCamera");
    editorCam.AddComponent<pwg::components::TransformComponent>();
    editorCam.AddComponent<pwg::components::CameraComponent>();
    editorCam.AddComponent<pwg::components::EditorCameraComponent>();

    m_terrain = std::make_unique<Terrain>(m_editorSceneRegistry, m_resourceManager, 200);
}

pwg::EditorScene::EditorScene(const EditorScene& otherEditorScene)
    : m_window(otherEditorScene.m_window),
      m_keyboardInput(otherEditorScene.m_keyboardInput),
      m_mouseInput(otherEditorScene.m_mouseInput), 
      m_renderer(otherEditorScene.m_renderer),
      m_resourceManager(otherEditorScene.m_resourceManager)
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
    pwg::systems::EditorCameraControllerSystem::Update(m_editorSceneRegistry, m_mouseInput, m_aspectRatio);
    m_terrain->Update();
}

void pwg::EditorScene::Draw()
{
    ImGui::Begin("Main View");

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("SceneRenderArea", windowSize, false,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    int width = static_cast<int>(windowSize.x);
    int height = static_cast<int>(windowSize.y);

    if (m_frameBuffer->GetWidth() != width || m_frameBuffer->GetHeight() != height)
    {
        m_frameBuffer->Resize(width, height);
    }

    if (height > 0)
    {
        m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    }

    pwg::components::CameraComponent* activeCamera = nullptr;
    auto camView = m_editorSceneRegistry.view<pwg::components::CameraComponent>();
    for (auto [entity, camera] : camView.each())
    {
        activeCamera = &camera;
        break;
    }

    if (!activeCamera) {
        std::cerr << "Brak aktywnej kamery w ECS!\n";
    }

    m_frameBuffer->Bind();
    m_renderer.Clear();
    m_renderer.Update(activeCamera, *m_terrain->GetMesh());
    m_renderer.Draw(*m_terrain->GetMesh());
    m_frameBuffer->Unbind();

    ImGui::Image(
        (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
        windowSize,
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::EndChild();
     
    ImGui::End(); // Scene view

    //do przeniesienia
    ImGui::Begin("Controls");

    if (ImGui::BeginTabBar("MainTabs"))
    {
        if (ImGui::BeginTabItem("Noise"))
        {

            auto m_noiseTexture = m_terrain->GetNoiseTexture();

            float amplitude = m_noiseTexture->GetNoiseParameters().amplitude;
            float frequency = m_noiseTexture->GetNoiseParameters().frequency;
            float scale = m_noiseTexture->GetNoiseParameters().scale;
            glm::vec2 offset = m_noiseTexture->GetNoiseParameters().offset;
            int seed = m_noiseTexture->GetNoiseParameters().seed;
            int octaves = m_noiseTexture->GetNoiseParameters().octaves;
            float persistance = m_noiseTexture->GetNoiseParameters().persistance;
            float lacunarity = m_noiseTexture->GetNoiseParameters().lacunarity;

            bool updated = false;

            if (ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 10.0f)) { m_noiseTexture->SetAmplitude(amplitude); updated = true; }
            if (ImGui::SliderFloat("Frequency", &frequency, 0.001f, 1.0f)) { m_noiseTexture->SetFrequency(frequency); updated = true; }
            if (ImGui::SliderFloat("Scale", &scale, 0.1f, 200.0f)) { m_noiseTexture->SetScale(scale); updated = true; }
            if (ImGui::SliderInt("Octaves", &octaves, 1, 8)) { m_noiseTexture->SetOctaves(octaves); updated = true; }
            if (ImGui::SliderFloat("Persistance", &persistance, 0.01f, 2.0f)) { m_noiseTexture->SetPersistance(persistance); updated = true; }
            if (ImGui::SliderFloat("Lacunarity", &lacunarity, 0.01f, 10.0f)) { m_noiseTexture->SetLacunarity(lacunarity); updated = true; }
            if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, 100.0f)) { m_noiseTexture->SetOffset(offset); updated = true; }
            if (ImGui::InputInt("Seed", &seed)) { m_noiseTexture->SetSeed(seed); updated = true; }

            if (updated)
                m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());

            ImVec2 windowSize = ImGui::GetContentRegionAvail();

            if (m_noiseTexture && m_noiseTexture->GetTextureID() != 0)
            {
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                ImVec2 noiseSize((float)m_noiseTexture->GetNoiseWidth(), (float)m_noiseTexture->GetNoiseHeight());

                //Center noise image on the window
                ImVec2 cursorPos = ImGui::GetCursorPos();
                ImVec2 offset((windowSize.x - noiseSize.x) * 0.5f, (windowSize.y - noiseSize.y) * 0.5f);

                ImGui::SetCursorPos(ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y));

                ImGui::Image(
                    (ImTextureID)(intptr_t)m_noiseTexture->GetTextureID(),
                    noiseSize,
                    ImVec2(0, 1), ImVec2(1, 0)
                );
            }
            else
            {
                ImGui::Text("Noise texture not initialized.");
            }

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    
    ImGui::End();

    //do przeniesienia
}

std::unique_ptr<pwg::IScene> pwg::EditorScene::Clone()
{
	return std::make_unique<pwg::EditorScene>(*this);
}
