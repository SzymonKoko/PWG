#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "scene/systems/editorCameraControllerSystem.h"
#include "scene/components/tagComponent.h"
#include "scene/systems/planeMeshSystem.h"


pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(800, 800, true);

    auto editorCam = CreateEntity("EditorCamera");
    editorCam.AddComponent<pwg::components::TransformComponent>();
    editorCam.AddComponent<pwg::components::CameraComponent>();
    editorCam.AddComponent<pwg::components::EditorCameraComponent>();

    auto planeMesh = CreateEntity("PlaneMesh");
    planeMesh.AddComponent<components::MeshComponent>();
    planeMesh.AddComponent<components::PlaneMeshComponent>(255, 255);

    m_noiseTexture = std::make_unique<NoiseTexture>();
    m_meshManager = std::make_unique<MeshManager>();
}

pwg::EditorScene::EditorScene(const EditorScene& otherEditorScene)
    : m_window(otherEditorScene.m_window),
      m_keyboardInput(otherEditorScene.m_keyboardInput),
      m_mouseInput(otherEditorScene.m_mouseInput), 
      m_renderer(otherEditorScene.m_renderer)
{
    if (otherEditorScene.m_frameBuffer)
    {
        m_frameBuffer = std::make_unique<FrameBuffer>(*otherEditorScene.m_frameBuffer);
    }

    if (otherEditorScene.m_noiseTexture)
    {
        m_noiseTexture = std::make_unique<NoiseTexture>(*otherEditorScene.m_noiseTexture);
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

    if (otherEditorScene.m_noiseTexture)
    {
        m_noiseTexture = std::make_unique<NoiseTexture>(*otherEditorScene.m_noiseTexture);
    }
    else
    {
        m_noiseTexture.reset();
    }

    return *this;
}

pwg::Entity pwg::EditorScene::CreateEntity(const std::string& name)
{
    Entity entity(m_editorSceneRegistry.create(), &m_editorSceneRegistry);
    entity.AddComponent<pwg::components::TagComponent>(name);
    return entity;
}

void pwg::EditorScene::Update(const float& dt)
{
    pwg::systems::EditorCameraControllerSystem::Update(m_editorSceneRegistry, m_mouseInput, m_aspectRatio);
    pwg::systems::PlaneMeshSystem::Update(m_editorSceneRegistry, *m_meshManager);

    if (m_noiseTexture)
    {
        m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());
    }

    
}

void pwg::EditorScene::Draw()
{
    ImGui::Begin("Main View");

    if (ImGui::BeginTabBar("MainTabs"))
    {
        if (ImGui::BeginTabItem("Scene"))
        {
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

            pwg::components::MeshComponent* meshComponent;
            pwg::components::PlaneMeshComponent* planeMeshComponent;
            auto meshView = m_editorSceneRegistry.view<components::MeshComponent, components::PlaneMeshComponent>();
            for (auto [entity, mesh, plane] : meshView.each())
            {
                meshComponent = &mesh;
                planeMeshComponent = &plane;
                break;
            }

            if (!meshComponent)
            {
                std::cerr << "Brak mesha\n";
            }

            if (!m_meshManager)
            {
                
            }
            pwg::Mesh* mesh = &m_meshManager->GetMesh(meshComponent->meshID);

            NoiseDeformer noiseDeformer;
            noiseDeformer.ApplyNoise(*planeMeshComponent, *mesh, m_noiseTexture->GetNoiseData());

            m_frameBuffer->Bind();
            m_renderer.Clear();
            m_renderer.Update(activeCamera, *mesh);
            m_renderer.Draw(*mesh);
            m_frameBuffer->Unbind();

            ImGui::Image(
                (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
                windowSize,
                ImVec2(0, 1), ImVec2(1, 0)
            );

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Noise"))
        {

            //Do usuniecia

            float amplitude = m_noiseTexture->GetNoiseParameters().amplitude;
            float frequency = m_noiseTexture->GetNoiseParameters().frequency;
            float scale = m_noiseTexture->GetNoiseParameters().scale;
            glm::vec2 offset = m_noiseTexture->GetNoiseParameters().offset;
            int seed = m_noiseTexture->GetNoiseParameters().seed;

            bool updated = false;

            if (ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 10.0f)) { m_noiseTexture->SetAmplitude(amplitude); updated = true; }
            if (ImGui::SliderFloat("Frequency", &frequency, 0.001f, 1.0f)) { m_noiseTexture->SetFrequency(frequency); updated = true; }
            if (ImGui::SliderFloat("Scale", &scale, 0.1f, 200.0f)) { m_noiseTexture->SetScale(scale); updated = true; }
            if (ImGui::DragFloat2("Offset", &offset.x, 0.01f)) { m_noiseTexture->SetOffset(offset); updated = true; }
            if (ImGui::InputInt("Seed", &seed)) { m_noiseTexture->SetSeed(seed); updated = true; }

            if (updated)
                m_noiseTexture->UpdateNoiseData(m_noiseTexture->GetNoiseParameters());

            //Do usuniecia


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

        ImGui::EndTabBar();
    }

    ImGui::End();
}

std::unique_ptr<pwg::IScene> pwg::EditorScene::Clone()
{
	return std::make_unique<pwg::EditorScene>(*this);
}
