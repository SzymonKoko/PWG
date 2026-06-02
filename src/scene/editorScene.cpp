#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "controls/noiseControls.h"
#include "controls/terrainLayerControls.h"
#include "controls/lightControls.h"


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
    std::vector<std::string> texturesPathsAlbedo;
    std::vector<std::string> texturesPathsNormal;
    std::vector<std::string> texturesPathsData;

    //Albedo
    texturesPathsAlbedo.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_Color.jpg");
    texturesPathsAlbedo.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_Color.jpg");
    texturesPathsAlbedo.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_Color.jpg");

    //Normal
    texturesPathsNormal.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_NormalGL.jpg");
    texturesPathsNormal.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_NormalGL.jpg");
    texturesPathsNormal.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_NormalGL.jpg");

    //Roughness
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_Roughness.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_Roughness.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_Roughness.jpg");

    //Metallic
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_Roughness.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_Roughness.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_Roughness.jpg");

    //AO
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_AmbientOcclusion.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_AmbientOcclusion.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_AmbientOcclusion.jpg");

    //Height
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_grass2/Grass002_2K-JPG_Displacement.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_stone2/Rock060_2K-JPG_Displacement.jpg");
    texturesPathsData.push_back(terrainTexturesPath + "pbr/pbr_sediment/Ground079S_2K-JPG_Displacement.jpg");


    textureManager->LoadTextureArray("terrainTexturesAlbedo", texturesPathsAlbedo, TextureType::ALBEDO);
    textureManager->LoadTextureArray("terrainTexturesNormal", texturesPathsNormal, TextureType::NORMAL);
    textureManager->LoadTextureArray("terrainTexturesData", texturesPathsData, TextureType::DATA);
    auto textureArrayAlbedo = textureManager->GetTextureArray("terrainTexturesAlbedo");
    auto textureArrayNormal = textureManager->GetTextureArray("terrainTexturesNormal");
    auto textureArrayData = textureManager->GetTextureArray("terrainTexturesData");

    std::vector<std::string> skyboxPaths = {
        terrainTexturesPath + "skybox/miramar_rt.tga",
        terrainTexturesPath + "skybox/miramar_lf.tga",
        terrainTexturesPath + "skybox/miramar_up.tga",
        terrainTexturesPath + "skybox/miramar_dn.tga",
        terrainTexturesPath + "skybox/miramar_bk.tga",
        terrainTexturesPath + "skybox/miramar_ft.tga",
    };

    textureManager->LoadCubeMapTexture("Skybox", skyboxPaths, TextureType::ALBEDO);


    /*== CREATING MATERIALS ==*/
    auto terrainMaterial = materialManager->CreateMaterial("TerrainMaterial", terrainShader);
    auto unlitMaterial = materialManager->CreateMaterial("UnlitMaterial", unlitShader);
    auto skyboxMaterial = materialManager->CreateMaterial("SkyboxMaterial", skyboxShader);

    terrainMaterial->SetTextureArray("u_TexturesAlbedo", textureArrayAlbedo);
    terrainMaterial->SetTextureArray("u_TexturesNormal", textureArrayNormal);
    terrainMaterial->SetTextureArray("u_TexturesData", textureArrayData);
    auto skybox = textureManager->GetCubeMapTexture("Skybox");

    m_terrain = std::make_shared<Terrain>(m_resourceManager);
    m_sunObject = std::make_shared<SunObject>(sunMesh, unlitMaterial);
    m_skybox = std::make_shared<SkyBox>(skybox, skyboxMaterial);

    m_cameraManager->SetCamera(CameraType::EDITOR);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());

    m_cameraManager->SetCamera(CameraType::FLYOVER);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());

    m_cameraManager->SetCamera(CameraType::PLAYER);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());
    m_cameraManager->SetTerrainHeightProvider([this](float x, float z)
    {
        return m_terrain->GetTerrainHeightAt(x, z);
    });

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
    else if (cameraType == CameraType::FLYOVER || cameraType == CameraType::PLAYER)
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

    m_renderer.SetDebugSettings(m_terrain->GetTerrainDebug()->GetDebugSettings());

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

    ImGui::Begin("Terrain Controls");

    if (ImGui::BeginTabBar("Terrain"));
    {
        controls::NoiseControls::ShowControl(m_terrain->GetChunkManager());
        controls::TerrainLayerControls::ShowControl(m_terrain->GetTerrainLayers());
    }
    ImGui::EndTabBar();

    ImGui::End();

    ImGui::Begin("Light Controls");

    if (ImGui::BeginTabBar("Light"))
    {
        controls::LightControls::ShowControl(m_sunObject);
    }
    ImGui::EndTabBar();

    ImGui::End();

    ImGui::Begin("Debug");

    if (ImGui::BeginTabBar("TerrainDebug"))
    {
        m_terrain->GetTerrainDebug()->ShowGUI();
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

    if (m_keyboardInput.IsPressed(Action::FlyoverCamera))
    {
        m_cameraManager->SetCamera(CameraType::FLYOVER);
    }

    if (m_keyboardInput.IsPressed(Action::PlayerCamera))
    {
        m_cameraManager->SetCamera(CameraType::PLAYER);
    }
}

