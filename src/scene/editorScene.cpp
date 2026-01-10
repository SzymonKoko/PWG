#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "scene/systems/editorCameraControllerSystem.h"
#include "scene/components/tagComponent.h"
#include "controls/noiseControls.h"
#include "controls/terrainLayerControls.h"


pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput),
      m_resourceManager(resourceManager),
      m_renderer(renderer)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(600, 600, true);

    m_cameraManager = std::make_unique<CameraManager>(m_mouseInput, m_keyboardInput);

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

    m_cameraManager->SetCamera(CameraType::EDITOR);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());

    PWG_INFO("Editor scene initialized");
}

void pwg::EditorScene::Update(const float& dt)
{
    HandleKeyboardInputs();
    m_cameraManager->Update(dt);
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

    m_frameBuffer->Bind();
    m_renderer.BeginFrame();
    m_renderer.SetCamera(m_cameraManager->GetActiveCamera());
    m_renderer.AddLight(m_sunObject->GetLight());
    m_renderer.AddToQueue(m_terrain.get());
    m_renderer.AddToQueue(m_sunObject.get());
    m_renderer.DrawAll();
    m_renderer.EndFrame();
    m_frameBuffer->Unbind();

    ImGui::Text("SDSDSDSDS");
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
        controls::TerrainLayerControls::ShowControl(m_terrain->GetTerrainLayers());
    } 
    ImGui::EndTabBar(); // MainTabs 
    
    ImGui::End(); //Controls
}

void pwg::EditorScene::HandleKeyboardInputs()
{
    if (m_keyboardInput.IsPressed(Action::EditorCamera))
    {
        m_cameraManager->SetCamera(CameraType::EDITOR);
    }

    if (m_keyboardInput.IsPressed(Action::PlayerCamera))
    {
        m_cameraManager->SetCamera(CameraType::PLAYER);
    }
}

