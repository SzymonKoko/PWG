#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "scene/systems/editorCameraControllerSystem.h"
#include "scene/components/tagComponent.h"
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

    /*== CREATING MESH ==*/
    m_resourceManager->GetMeshManager().CreatePlaneMesh(2000, "PlaneMesh");
    m_resourceManager->GetMeshManager().CreateSphereMesh(128.0f, 36, 72, "SunMesh");

    auto shaderManager = m_resourceManager->GetShaderManager();
    auto textureManager = m_resourceManager->GetTextureManager();

    /*== SHADER LOADING ==*/
    std::string terrainShadersPath = "../assets/shaders/terrainShaders/";
    std::string unlitShadersPath = "../assets/shaders/unlit/";

    shaderManager.Load("terrainShader", terrainShadersPath + "terrain.vert", terrainShadersPath + "terrain.frag");
    shaderManager.Load("unlitShader", unlitShadersPath + "unlit.vert", unlitShadersPath + "unlit.frag");
    shaderManager.LoadCompute("heightmapComputeShader", terrainShadersPath + "terrain_height.comp");
    shaderManager.LoadCompute("normalmapComputeShader", terrainShadersPath + "terrain_normal.comp");
    shaderManager.LoadCompute("splatmapComputeShader", terrainShadersPath + "terrain_splat.comp");

    /*== TEXTURE LOADING ==*/
    std::string terrainTexturesPath = "../assets/textures/";
    std::vector<std::string> texturesPaths;

    texturesPaths.push_back(terrainTexturesPath + "grass.png");
    texturesPaths.push_back(terrainTexturesPath + "dirt.png");
    texturesPaths.push_back(terrainTexturesPath + "stone.png");
    texturesPaths.push_back(terrainTexturesPath + "snow.png");

    textureManager.LoadTextureArray("terrainTextures", texturesPaths);
    auto textureArray = textureManager.GetTextureArray("terrainTextures");

    auto terrainShader = shaderManager.GetShader<pwg::Shader>("terrainShader");
    auto unlitShader = shaderManager.GetShader<pwg::Shader>("unlitShader");

    /*== MESH LOADING ==*/
    auto terrainMesh = m_resourceManager->GetMeshManager().GetMesh("PlaneMesh");
    auto sunMesh = m_resourceManager->GetMeshManager().GetMesh("SunMesh");

    /*== CREATING MATERIALS ==*/
    auto terrainMaterial = m_resourceManager->GetMaterialManager().CreateMaterial("TerrainMaterial", terrainShader);
    auto unlitMaterial = m_resourceManager->GetMaterialManager().CreateMaterial("UnlitMaterial", unlitShader);

    /*== INITIALIZING TERRAIN ==*/
    TerrainComputeShaders terrainComputeShaders(
        shaderManager.GetShader<pwg::ComputeShader>("heightmapComputeShader"),
        shaderManager.GetShader<pwg::ComputeShader>("normalmapComputeShader"),
        shaderManager.GetShader<pwg::ComputeShader>("splatmapComputeShader")
    );

    terrainMaterial->SetTextureArray("u_Textures", textureArray);

    m_terrain = std::make_shared<Terrain>(terrainMesh, terrainMaterial, terrainComputeShaders);
    m_sunObject = std::make_shared<SunObject>(sunMesh, unlitMaterial);

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
    pwg::systems::EditorCameraControllerSystem::Update(m_editorSceneRegistry, m_mouseInput, m_keyboardInput, m_aspectRatio ,dt);
    m_terrain->Update(dt);
    m_sunObject->Update(dt);
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
    m_renderer.AddToQueue(m_sunObject.get());
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
