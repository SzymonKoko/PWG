#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
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
    m_resourceManager->GetMeshManager()->CreateSphereMesh(128.0f, 36, 72, "SunMesh");

    auto shaderManager = m_resourceManager->GetShaderManager();
    auto textureManager = m_resourceManager->GetTextureManager();
    auto materialManager = m_resourceManager->GetMaterialManager();

    /*== SHADER LOADING ==*/
    std::string terrainShadersPath = "../assets/shaders/terrainShaders/";
    std::string unlitShadersPath = "../assets/shaders/unlit/";
    std::string skyboxShadersPath = "../assets/shaders/skybox/";

    shaderManager->Load("terrainShader", terrainShadersPath + "terrain.vert", terrainShadersPath + "terrain.frag");
    shaderManager->Load("unlitShader", unlitShadersPath + "unlit.vert", unlitShadersPath + "unlit.frag");
    shaderManager->Load("skyboxShader", skyboxShadersPath + "skybox.vert", skyboxShadersPath + "skybox.frag");

    auto terrainShader = shaderManager->GetShader<pwg::Shader>("terrainShader");
    auto unlitShader = shaderManager->GetShader<pwg::Shader>("unlitShader");
    auto skyboxShader = shaderManager->GetShader<pwg::Shader>("skyboxShader");

    /*== MESH LOADING ==*/
    auto sunMesh = m_resourceManager->GetMeshManager()->GetMesh("SunMesh");

    /*== TEXTURE LOADING ==*/
    std::string terrainTexturesPath = "../assets/textures/";
    std::vector<std::string> texturesPaths;

    texturesPaths.push_back(terrainTexturesPath + "grasschat3.png");
    texturesPaths.push_back(terrainTexturesPath + "sediment.png");
    texturesPaths.push_back(terrainTexturesPath + "stonechat.png");
    texturesPaths.push_back(terrainTexturesPath + "snow.png");

    textureManager->LoadTextureArray("terrainTextures", texturesPaths);
    auto textureArray = textureManager->GetTextureArray("terrainTextures");

    std::vector<std::string> skyboxPaths = {
        terrainTexturesPath + "skybox/miramar_rt.tga",
        terrainTexturesPath + "skybox/miramar_lf.tga",
        terrainTexturesPath + "skybox/miramar_up.tga",
        terrainTexturesPath + "skybox/miramar_dn.tga",
        terrainTexturesPath + "skybox/miramar_bk.tga",
        terrainTexturesPath + "skybox/miramar_ft.tga",
    };

    textureManager->LoadCubeMapTexture("Skybox", skyboxPaths);


    /*== CREATING MATERIALS ==*/
    auto terrainMaterial = materialManager->CreateMaterial("TerrainMaterial", terrainShader);
    auto unlitMaterial = m_resourceManager->GetMaterialManager()->CreateMaterial("UnlitMaterial", unlitShader);
    auto skyboxMaterial = materialManager->CreateMaterial("SkyboxMaterial", skyboxShader);

    terrainMaterial->SetTextureArray("u_Textures", textureArray);
    auto skybox = textureManager->GetCubeMapTexture("Skybox");

    m_terrain = std::make_shared<Terrain>(m_resourceManager);
    m_sunObject = std::make_shared<SunObject>(sunMesh, unlitMaterial);
    m_skybox = std::make_shared<SkyBox>(skybox, skyboxMaterial);

    m_cameraManager->SetCamera(CameraType::EDITOR);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());

    m_cameraManager->SetCamera(CameraType::PLAYER);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());

    m_cameraManager->SetCamera(CameraType::EDITOR);

    PWG_INFO("Editor scene initialized");
}

void pwg::EditorScene::Update(const float& dt)
{
    HandleKeyboardInputs();

    auto cameraType = m_cameraManager->GetActiveCameraType();
    if (cameraType == CameraType::EDITOR)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if (cameraType == CameraType::PLAYER)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    m_cameraManager->Update(dt);
    auto activeCamera = m_cameraManager->GetActiveCamera();
    m_terrain->Update(dt ,activeCamera);
    m_sunObject->Update(dt, activeCamera);

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
    m_renderer.AddSkyBox(m_skybox);
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

    ImGui::End();

    ImGui::Begin("Controls");

    if (ImGui::BeginTabBar("MainTabs"))
    {
        controls::NoiseControls::ShowControl(m_terrain->GetChunkManager());
        controls::TerrainLayerControls::ShowControl(m_terrain->GetTerrainLayers());
    }
    ImGui::EndTabBar();

    ImGui::End();
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

