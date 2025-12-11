#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "scene/systems/editorCameraControllerSystem.h"
#include "scene/components/tagComponent.h"
#include "scene/systems/planeMeshSystem.h"
#include "controls/noiseControls.h"


pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput),
      m_resourceManager(resourceManager),
      m_renderer(renderer)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(600, 600, true);

    Entity editorCam(&m_editorSceneRegistry, "EditorCamera");
    editorCam.AddComponent<pwg::components::TransformComponent>();
    editorCam.AddComponent<pwg::components::CameraComponent>();
    editorCam.AddComponent<pwg::components::EditorCameraComponent>();

    auto terrainShader = m_resourceManager->GetShaderManager().GetShader<pwg::Shader>("default");
    auto mesh = m_resourceManager->GetMeshManager().GetMesh("PlaneMesh");
    auto material = m_resourceManager->GetMaterialManager().CreateMaterial("TerrainMaterial", terrainShader);
    auto noiseComputeShader = m_resourceManager->GetShaderManager().GetShader<pwg::ComputeShader>("noise");

    m_terrain = std::make_shared<Terrain>(mesh, material, noiseComputeShader);

    pwg::systems::EditorCameraControllerSystem::SetCameraDefaultPosition(m_editorSceneRegistry, m_terrain->GetSize());

    PWG_INFO("Editor scene initialized");
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
    m_terrain->Update(dt);
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
    m_renderer.BeginFrame();
    m_renderer.SetCamera(activeCamera);
    m_renderer.AddToQueue(m_terrain.get());
    m_renderer.DrawAll();
    m_renderer.EndFrame();
    m_frameBuffer->Unbind();

    ImGui::Image(
        (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
        windowSize,
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::EndChild();
     
    ImGui::End(); // Scene view

    ImGui::Begin("Controls"); 

    if (ImGui::BeginTabBar("MainTabs"))
    {
        auto& settings = m_terrain->GetNoiseSettings();
        auto size = m_terrain->GetSize();
        controls::NoiseControls::ShowControl(settings, size);
    } 
    ImGui::EndTabBar(); // MainTabs 
    
    ImGui::End(); //Controls
}

std::unique_ptr<pwg::IScene> pwg::EditorScene::Clone()
{
	return std::make_unique<pwg::EditorScene>(*this);
}
