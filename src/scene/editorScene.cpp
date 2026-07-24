#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "controls/noiseControls.h"
#include "controls/terrainGraphEditor.h"
#include "controls/environmentControls.h"
#include "platform/nativeDialogs.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <map>
#include <limits>
#include <numbers>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace
{
	std::string NormalizePath(const std::string& path)
	{
		return std::filesystem::path(path).generic_string();
	}

	std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	std::array<const char*, 6> SkyboxFaceSuffixes()
	{
		return { "rt", "lf", "up", "dn", "bk", "ft" };
	}

	pwg::SkyboxDefinition DefaultSkyboxDefinition()
	{
		pwg::SkyboxDefinition definition;
		definition.name = "miramar";
		definition.folder = "../assets/textures/skybox";
		definition.faces = {
			"../assets/textures/skybox/miramar_rt.tga",
			"../assets/textures/skybox/miramar_lf.tga",
			"../assets/textures/skybox/miramar_up.tga",
			"../assets/textures/skybox/miramar_dn.tga",
			"../assets/textures/skybox/miramar_bk.tga",
			"../assets/textures/skybox/miramar_ft.tga"
		};
		return definition;
	}

	bool IsSupportedSkyboxTexture(const std::filesystem::path& path)
	{
		std::string extension = ToLower(path.extension().string());
		return extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".tga";
	}

	const std::array<std::vector<std::string>, 6>& SkyboxFaceAliases()
	{
		static const std::array<std::vector<std::string>, 6> aliases = {
			std::vector<std::string>{ "rt", "right", "px", "posx", "positivex", "xpos", "xpositive" },
			std::vector<std::string>{ "lf", "left", "nx", "negx", "negativex", "xneg", "xnegative" },
			std::vector<std::string>{ "up", "top", "py", "posy", "positivey", "ypos", "ypositive" },
			std::vector<std::string>{ "dn", "down", "bottom", "ny", "negy", "negativey", "yneg", "ynegative" },
			std::vector<std::string>{ "bk", "back", "pz", "posz", "positivez", "zpos", "zpositive" },
			std::vector<std::string>{ "ft", "front", "nz", "negz", "negativez", "zneg", "znegative" }
		};
		return aliases;
	}

	bool TryMatchSkyboxFace(const std::string& stem, int& faceIndex, std::string& groupName)
	{
		std::string lowerStem = ToLower(stem);
		const auto& aliases = SkyboxFaceAliases();
		const std::vector<std::string> separators = { "_", "-", " " };

		for (int i = 0; i < static_cast<int>(aliases.size()); ++i)
		{
			for (const auto& alias : aliases[i])
			{
				if (lowerStem == alias)
				{
					faceIndex = i;
					groupName = "skybox";
					return true;
				}

				for (const auto& separator : separators)
				{
					std::string suffix = separator + alias;
					if (lowerStem.size() > suffix.size() && lowerStem.ends_with(suffix))
					{
						faceIndex = i;
						groupName = stem.substr(0, stem.size() - suffix.size());
						return true;
					}

					std::string prefix = alias + separator;
					if (lowerStem.size() > prefix.size() && lowerStem.starts_with(prefix))
					{
						faceIndex = i;
						groupName = stem.substr(prefix.size());
						return true;
					}
				}
			}
		}

		return false;
	}

	std::vector<pwg::SkyboxDefinition> FindSkyboxDefinitions(const std::string& folder)
	{
		std::vector<pwg::SkyboxDefinition> definitions;
		if (folder.empty() || !std::filesystem::is_directory(folder))
		{
			return definitions;
		}

		std::map<std::string, std::array<std::string, 6>> groupedFaces;
		for (const auto& entry : std::filesystem::directory_iterator(folder))
		{
			if (!entry.is_regular_file() || !IsSupportedSkyboxTexture(entry.path()))
			{
				continue;
			}

			std::string stem = entry.path().stem().string();
			int faceIndex = -1;
			std::string groupName;
			if (TryMatchSkyboxFace(stem, faceIndex, groupName))
			{
				groupedFaces[groupName][faceIndex] = NormalizePath(entry.path().string());
			}
		}

		for (const auto& [name, faces] : groupedFaces)
		{
			bool complete = true;
			for (const auto& face : faces)
			{
				complete = complete && !face.empty();
			}
			if (!complete)
			{
				continue;
			}

			pwg::SkyboxDefinition definition;
			definition.name = name;
			definition.folder = NormalizePath(folder);
			definition.faces = faces;
			definitions.push_back(definition);
		}
		return definitions;
	}

	std::vector<std::string> FaceVector(const pwg::SkyboxDefinition& definition)
	{
		return {
			definition.faces[0],
			definition.faces[1],
			definition.faces[2],
			definition.faces[3],
			definition.faces[4],
			definition.faces[5]
		};
	}

	std::string ImportedSkyboxFolder(const std::string& imagePath)
	{
		std::string stem = std::filesystem::path(imagePath).stem().string();
		for (char& c : stem)
		{
			if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
			{
				c = '_';
			}
		}
		return NormalizePath("../assets/cache/skybox_imports/" + stem);
	}

	void CopyFaceRegion(const unsigned char* source, int sourceWidth, int sourceHeight, int sourceX, int sourceY, int faceSize, std::vector<unsigned char>& face)
	{
		face.resize(static_cast<std::size_t>(faceSize) * static_cast<std::size_t>(faceSize) * 4);
		for (int y = 0; y < faceSize; ++y)
		{
			for (int x = 0; x < faceSize; ++x)
			{
				int sx = std::clamp(sourceX + x, 0, sourceWidth - 1);
				int sy = std::clamp(sourceY + y, 0, sourceHeight - 1);
				const unsigned char* src = source + (static_cast<std::size_t>(sy) * sourceWidth + sx) * 4;
				unsigned char* dst = face.data() + (static_cast<std::size_t>(y) * faceSize + x) * 4;
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = src[3];
			}
		}
	}

	glm::vec3 CubemapDirection(int faceIndex, float u, float v)
	{
		float x = 2.0f * u - 1.0f;
		float y = 2.0f * v - 1.0f;
		switch (faceIndex)
		{
		case 0: return glm::normalize(glm::vec3(1.0f, -y, -x));  // +X rt
		case 1: return glm::normalize(glm::vec3(-1.0f, -y, x));  // -X lf
		case 2: return glm::normalize(glm::vec3(x, 1.0f, y));    // +Y up
		case 3: return glm::normalize(glm::vec3(x, -1.0f, -y));  // -Y dn
		case 4: return glm::normalize(glm::vec3(-x, -y, 1.0f));  // +Z bk
		default: return glm::normalize(glm::vec3(x, -y, -1.0f)); // -Z ft
		}
	}

	void SampleBilinear(const unsigned char* source, int width, int height, float u, float v, unsigned char* out)
	{
		u = u - std::floor(u);
		v = std::clamp(v, 0.0f, 1.0f);
		float x = u * static_cast<float>(width - 1);
		float y = v * static_cast<float>(height - 1);
		int x0 = static_cast<int>(std::floor(x));
		int y0 = static_cast<int>(std::floor(y));
		int x1 = (x0 + 1) % width;
		int y1 = std::min(y0 + 1, height - 1);
		float tx = x - static_cast<float>(x0);
		float ty = y - static_cast<float>(y0);

		for (int c = 0; c < 4; ++c)
		{
			float c00 = source[(static_cast<std::size_t>(y0) * width + x0) * 4 + c];
			float c10 = source[(static_cast<std::size_t>(y0) * width + x1) * 4 + c];
			float c01 = source[(static_cast<std::size_t>(y1) * width + x0) * 4 + c];
			float c11 = source[(static_cast<std::size_t>(y1) * width + x1) * 4 + c];
			float top = c00 + (c10 - c00) * tx;
			float bottom = c01 + (c11 - c01) * tx;
			out[c] = static_cast<unsigned char>(std::clamp(top + (bottom - top) * ty, 0.0f, 255.0f));
		}
	}

	void RenderEquirectFace(const unsigned char* source, int sourceWidth, int sourceHeight, int faceIndex, int faceSize, std::vector<unsigned char>& face)
	{
		face.resize(static_cast<std::size_t>(faceSize) * static_cast<std::size_t>(faceSize) * 4);
		const float pi = std::numbers::pi_v<float>;
		for (int y = 0; y < faceSize; ++y)
		{
			for (int x = 0; x < faceSize; ++x)
			{
				float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize);
				float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize);
				glm::vec3 direction = CubemapDirection(faceIndex, u, v);
				float longitude = std::atan2(direction.z, direction.x);
				float latitude = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
				float sourceU = 0.5f + longitude / (2.0f * pi);
				float sourceV = 0.5f - latitude / pi;
				unsigned char* dst = face.data() + (static_cast<std::size_t>(y) * faceSize + x) * 4;
				SampleBilinear(source, sourceWidth, sourceHeight, sourceU, sourceV, dst);
			}
		}
	}

	bool WriteSkyboxFaces(const std::string& outputFolder, const std::string& name, const std::array<std::vector<unsigned char>, 6>& faces, int faceSize, std::array<std::string, 6>& outputPaths)
	{
		std::filesystem::create_directories(outputFolder);
		auto suffixes = SkyboxFaceSuffixes();
		for (int i = 0; i < 6; ++i)
		{
			std::string path = NormalizePath(outputFolder + "/" + name + "_" + suffixes[i] + ".png");
			if (stbi_write_png(path.c_str(), faceSize, faceSize, 4, faces[i].data(), faceSize * 4) == 0)
			{
				return false;
			}
			outputPaths[i] = path;
		}
		return true;
	}

	bool HasFaceTransforms(const pwg::SkyboxDefinition& definition)
	{
		for (int i = 0; i < 6; ++i)
		{
			if ((definition.rotations[i] % 4) != 0 || definition.flipHorizontal[i] || definition.flipVertical[i])
			{
				return true;
			}
		}
		return false;
	}

	void TransformFaceImage(const unsigned char* source, int size, int rotation, bool flipHorizontal, bool flipVertical, std::vector<unsigned char>& output)
	{
		output.resize(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4);
		rotation = ((rotation % 4) + 4) % 4;

		for (int y = 0; y < size; ++y)
		{
			for (int x = 0; x < size; ++x)
			{
				int sx = x;
				int sy = y;

				if (flipHorizontal)
				{
					sx = size - 1 - sx;
				}
				if (flipVertical)
				{
					sy = size - 1 - sy;
				}

				int rx = sx;
				int ry = sy;
				switch (rotation)
				{
				case 1:
					rx = sy;
					ry = size - 1 - sx;
					break;
				case 2:
					rx = size - 1 - sx;
					ry = size - 1 - sy;
					break;
				case 3:
					rx = size - 1 - sy;
					ry = sx;
					break;
				default:
					break;
				}

				const unsigned char* src = source + (static_cast<std::size_t>(ry) * size + rx) * 4;
				unsigned char* dst = output.data() + (static_cast<std::size_t>(y) * size + x) * 4;
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = src[3];
			}
		}
	}

	std::vector<std::string> BuildSkyboxLoadFaces(const pwg::SkyboxDefinition& definition, std::string& status)
	{
		std::vector<std::string> sourceFaces = FaceVector(definition);
		if (!HasFaceTransforms(definition))
		{
			return sourceFaces;
		}

		std::string outputFolder = NormalizePath("../assets/cache/skybox_oriented/" + definition.name);
		std::filesystem::create_directories(outputFolder);
		auto suffixes = SkyboxFaceSuffixes();
		std::vector<std::string> transformedFaces(6);

		for (int i = 0; i < 6; ++i)
		{
			int width = 0;
			int height = 0;
			int channels = 0;
			stbi_set_flip_vertically_on_load(false);
			unsigned char* image = stbi_load(definition.faces[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!image || width != height)
			{
				if (image)
				{
					stbi_image_free(image);
				}
				status = "Cannot orient skybox face: " + definition.faces[i];
				return {};
			}

			std::vector<unsigned char> transformed;
			TransformFaceImage(image, width, definition.rotations[i], definition.flipHorizontal[i], definition.flipVertical[i], transformed);
			stbi_image_free(image);

			std::string path = NormalizePath(outputFolder + "/" + definition.name + "_" + suffixes[i] + ".png");
			if (stbi_write_png(path.c_str(), width, height, 4, transformed.data(), width * 4) == 0)
			{
				status = "Failed to write oriented skybox face";
				return {};
			}
			transformedFaces[i] = path;
		}

		status = "Skybox loaded with face orientation overrides: " + definition.name;
		return transformedFaces;
	}
}

pwg::EditorScene::EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer)
	: m_window(window), 
      m_keyboardInput(kinput), 
      m_mouseInput(minput),
      m_resourceManager(resourceManager),
      m_renderer(renderer)
{
	m_frameBuffer = std::make_unique<FrameBuffer>(600, 600, true, TextureType::SDR_COLOR);
	m_hdrFrameBuffer = std::make_unique<FrameBuffer>(600, 600, true, TextureType::HDR_COLOR);
	m_environment = std::make_shared<EnvironmentSystem>();

    m_cameraManager = std::make_unique<CameraManager>(m_mouseInput, m_keyboardInput);

    auto shaderManager = m_resourceManager->GetShaderManager();
    auto textureManager = m_resourceManager->GetTextureManager();
    auto materialManager = m_resourceManager->GetMaterialManager();

    /*== SHADER LOADING ==*/
    std::string terrainShadersPath = "../assets/shaders/terrainShaders/";
    std::string skyboxShadersPath = "../assets/shaders/skybox/";
    std::string postprocessShadersPath = "../assets/shaders/postprocess/";
    std::string shadowShadersPath = "../assets/shaders/shadow/";
    std::string objectShadersPath = "../assets/shaders/object/";

    shaderManager->Load("terrainShader", terrainShadersPath + "terrain.vert", terrainShadersPath + "terrain.frag");
    shaderManager->Load("skyboxShader", skyboxShadersPath + "skybox.vert", skyboxShadersPath + "skybox.frag");
    shaderManager->Load("postprocessShader", postprocessShadersPath + "postprocess.vert", postprocessShadersPath + "postprocess.frag");
    shaderManager->Load("terrainShadowShader", shadowShadersPath + "terrain_shadow.vert", shadowShadersPath + "terrain_shadow.frag");
    shaderManager->Load("objectShader", objectShadersPath + "object.vert", objectShadersPath + "object.frag");

    auto terrainShader = shaderManager->GetShader<pwg::Shader>("terrainShader");
    auto skyboxShader = shaderManager->GetShader<pwg::Shader>("skyboxShader");
    auto postprocessShader = shaderManager->GetShader<pwg::Shader>("postprocessShader");
    m_terrainShadowShader = shaderManager->GetShader<pwg::Shader>("terrainShadowShader");
    auto objectShader = shaderManager->GetShader<pwg::Shader>("objectShader");

    /*== TEXTURE LOADING ==*/
    std::string terrainTexturesPath = "../assets/textures/";
    m_skyboxDefinition = DefaultSkyboxDefinition();
    textureManager->LoadCubeMapTexture("Skybox", FaceVector(m_skyboxDefinition), TextureType::ALBEDO);


    /*== CREATING MATERIALS ==*/
    auto terrainMaterial = materialManager->CreateMaterial("TerrainMaterial", terrainShader);
    auto skyboxMaterial = materialManager->CreateMaterial("SkyboxMaterial", skyboxShader);
    auto objectMaterial = materialManager->CreateMaterial("ObjectMaterial", objectShader);

    m_terrainMaterialLibrary = std::make_shared<TerrainMaterialLibrary>(m_resourceManager, terrainMaterial);
    m_worldObjectManager = std::make_shared<WorldObjectManager>(objectMaterial);
    auto skybox = textureManager->GetCubeMapTexture("Skybox");

    m_terrain = std::make_shared<Terrain>(m_resourceManager);
    m_skybox = std::make_shared<SkyBox>(skybox, skyboxMaterial);
    m_postProcessor = std::make_shared<PostProcessor>(postprocessShader);
    m_shadowMap = std::make_shared<DirectionalShadowMap>();

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

    if (m_terrainGraphInputCaptured || m_viewportInputCaptured)
    {
        m_mouseInput.ResetScrollOffset();
    }
    else
    {
        m_cameraManager->Update(dt);
    }
    auto activeCamera = m_cameraManager->GetActiveCamera();
    if (activeCamera)
    {
        activeCamera->SetAspectRatio(m_aspectRatio);
    }
    m_terrain->Update(dt ,activeCamera);
    if (m_worldObjectManager)
    {
        m_worldObjectManager->UpdateTerrainSnap(m_terrain);
    }
    m_environment->Update(dt);

}

void pwg::EditorScene::Draw()
{
    if (m_assetImportActive)
    {
        DrawAssetImportWorkspace();
        return;
    }
    DrawOutliner();
    m_viewportInputCaptured = false;

    m_showMainView = true;
    {
    ImGui::Begin("Main View", nullptr, ImGuiWindowFlags_NoCollapse);

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
    if (m_hdrFrameBuffer->GetWidth() != width || m_hdrFrameBuffer->GetHeight() != height)
    {
        m_hdrFrameBuffer->Resize(width, height);
    }

    if (height > 0)
    {
        m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    }

    m_renderer.SetDebugSettings(m_terrain->GetTerrainDebug()->GetDebugSettings());

    auto environmentLighting = m_environment->GetLighting();
    auto activeCamera = m_cameraManager->GetActiveCamera();
    if (activeCamera)
    {
        activeCamera->SetAspectRatio(m_aspectRatio);
    }

    bool shadowsEnabled = environmentLighting.shadowsEnabled && environmentLighting.sunIntensity > 0.001f;
    if (shadowsEnabled)
    {
        m_shadowMap->Resize(environmentLighting.shadowResolution);
        for (int cascade = 0; cascade < DirectionalShadowMap::CascadeCount; ++cascade)
        {
            m_shadowMap->BeginCascade(cascade, environmentLighting, activeCamera);
            m_terrain->DrawShadowMap(m_terrainShadowShader, m_shadowMap->GetLightSpaceMatrix(cascade));
            m_shadowMap->End(width, height);
        }
    }

    std::array<glm::mat4, 3> shadowMatrices{
        m_shadowMap->GetLightSpaceMatrix(0),
        m_shadowMap->GetLightSpaceMatrix(1),
        m_shadowMap->GetLightSpaceMatrix(2)
    };
    std::array<unsigned int, 3> shadowTextures{
        m_shadowMap->GetDepthTextureID(0),
        m_shadowMap->GetDepthTextureID(1),
        m_shadowMap->GetDepthTextureID(2)
    };
    std::array<float, 3> cascadeSplits{
        m_shadowMap->GetCascadeSplit(0),
        m_shadowMap->GetCascadeSplit(1),
        m_shadowMap->GetCascadeSplit(2)
    };

    m_hdrFrameBuffer->Bind();
    m_renderer.BeginFrame();
    m_renderer.SetCamera(activeCamera);
    m_renderer.SetEnvironmentLighting(environmentLighting);
    m_renderer.SetShadowMaps(shadowMatrices, shadowTextures, cascadeSplits, shadowsEnabled);
    m_renderer.AddSkyBox(m_skybox);
    m_renderer.AddToQueue(m_terrain.get());
    m_renderer.DrawAll();
    if (m_worldObjectManager)
    {
        m_worldObjectManager->Draw(m_renderer);
        if (m_objectPlacementActive)
        {
            m_worldObjectManager->DrawPreview(m_renderer, m_placementAssetId, m_placementPreviewTransform, m_placementPreviewValid);
        }
    }
    m_renderer.EndFrame();
    m_hdrFrameBuffer->Unbind();

    m_frameBuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_postProcessor->Render(m_hdrFrameBuffer->GetTextureID(), m_environment->GetSettings(), width, height);
    m_frameBuffer->Unbind();

    ImGui::Image(
        (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
        windowSize,
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImVec2 viewportMin = ImGui::GetItemRectMin();
    ImVec2 viewportSize = ImGui::GetItemRectSize();
    ViewportPlacementInput placementInput = GetViewportPlacementInput(viewportMin, viewportSize);
    UpdateObjectPlacement(placementInput, activeCamera);
    if (m_objectPlacementActive)
    {
        m_viewportInputCaptured = true;
    }


    ImGui::EndChild();

    ImGui::End();
    }

    if (m_showTerrainControls)
    {
    ImGui::Begin("Terrain Controls", &m_showTerrainControls);

    if (ImGui::BeginTabBar("Terrain"))
    {
        controls::NoiseControls::ShowControl(m_terrain->GetChunkManager());
        ImGui::EndTabBar();
    }
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_dirty = true;
    }

    ImGui::End();
    }

    if (m_showDebugWindow)
    {
    ImGui::Begin("Debug", &m_showDebugWindow);

    if (ImGui::BeginTabBar("TerrainDebug"))
    {
        if (ImGui::BeginTabItem("Viewport"))
        {
            m_terrain->GetTerrainDebug()->ShowGUI();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Masks"))
        {
            controls::ShowMaskPreviews(m_terrain->GetChunkManager());
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    }

    if (m_showTerrainGraph)
    {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.10f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.08f, 0.26f));
    ImGui::Begin("Terrain Graph", &m_showTerrainGraph);
    bool terrainGraphInputCaptured = false;
    bool terrainGraphViewChanged = false;
    if (controls::TerrainGraphEditor::Show(m_terrain->GetChunkManager(), m_terrainGraphViewState, &terrainGraphInputCaptured, &terrainGraphViewChanged))
    {
        m_dirty = true;
    }
    if (terrainGraphViewChanged)
    {
        m_dirty = true;
    }
    m_terrainGraphInputCaptured = terrainGraphInputCaptured;
    ImGui::End();
    ImGui::PopStyleColor(2);
    }
    else
    {
        m_terrainGraphInputCaptured = false;
    }

    if (m_showMaterialLibrary)
    {
    ImGui::Begin("Material Library", &m_showMaterialLibrary);

    if (m_terrainMaterialLibrary && m_terrainMaterialLibrary->ShowGui())
    {
        m_dirty = true;
    }

    ImGui::End();
    }

    DrawWorldWindow();
    DrawObjectsWindow();
}

void pwg::EditorScene::NewProject()
{
    m_terrain->GetChunkManager()->ConfigureTerrain(31, 129);
    if (auto terrainPipeline = m_terrain->GetChunkManager()->GetTerrainComputePipeline())
    {
        terrainPipeline->ResetUniversalGraphToDefault();
    }
    m_terrainGraphViewState = controls::TerrainGraphViewState{};

    m_environment->ResetToDefaults();
    if (m_worldObjectManager)
    {
        m_worldObjectManager->Clear();
    }
    m_selectedRuleSetId = 0;
    m_selectedExclusionZoneId = 0;
    CloseAssetImport();

    ResetSkyboxToDefault();
    m_terrain->GetChunkManager()->Regenerate();
    ResetEditorWindows();
    m_cameraManager->SetCamera(CameraType::EDITOR);
    m_cameraManager->SetDefaultCameraPosition(m_terrain->GetSize());
    m_dirty = false;
}

std::shared_ptr<pwg::Terrain> pwg::EditorScene::GetTerrain()
{
    return m_terrain;
}

std::shared_ptr<pwg::EnvironmentSystem> pwg::EditorScene::GetEnvironmentSystem()
{
    return m_environment;
}

pwg::CameraManager* pwg::EditorScene::GetCameraManager()
{
    return m_cameraManager.get();
}

std::shared_ptr<pwg::TerrainMaterialLibrary> pwg::EditorScene::GetTerrainMaterialLibrary()
{
    return m_terrainMaterialLibrary;
}

const pwg::SkyboxDefinition& pwg::EditorScene::GetSkyboxDefinition() const
{
    return m_skyboxDefinition;
}

void pwg::EditorScene::SetSkyboxDefinition(const SkyboxDefinition& definition)
{
    m_skyboxDefinition = definition;
    ReloadSkybox();
}

bool pwg::EditorScene::ReloadSkybox()
{
	for (const auto& face : m_skyboxDefinition.faces)
    {
        if (face.empty() || !std::filesystem::exists(face))
        {
            m_skyboxStatus = "Skybox missing face: " + face;
            return false;
        }
    }

    std::string transformStatus;
    std::vector<std::string> loadFaces = BuildSkyboxLoadFaces(m_skyboxDefinition, transformStatus);
    if (loadFaces.empty())
    {
        m_skyboxStatus = transformStatus.empty() ? "Skybox orientation failed" : transformStatus;
        return false;
    }

    auto textureManager = m_resourceManager->GetTextureManager();
    textureManager->ReloadCubeMapTexture("Skybox", loadFaces, TextureType::ALBEDO);
    m_skybox->SetCubeMapTexture(textureManager->GetCubeMapTexture("Skybox"));
    m_skyboxStatus = transformStatus.empty() ? "Skybox loaded: " + m_skyboxDefinition.name : transformStatus;
    return true;
}

bool pwg::EditorScene::ImportSkyboxImage(const std::string& imagePath)
{
    if (imagePath.empty() || !std::filesystem::exists(imagePath))
    {
        m_skyboxStatus = "Skybox image does not exist";
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* image = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!image)
    {
        m_skyboxStatus = "Failed to read skybox image";
        return false;
    }

    std::array<std::vector<unsigned char>, 6> faces;
    int faceSize = 0;
    std::string layoutName;

    if (width == height * 6)
    {
        layoutName = "strip_h";
        faceSize = height;
        for (int i = 0; i < 6; ++i)
        {
            CopyFaceRegion(image, width, height, i * faceSize, 0, faceSize, faces[i]);
        }
    }
    else if (height == width * 6)
    {
        layoutName = "strip_v";
        faceSize = width;
        for (int i = 0; i < 6; ++i)
        {
            CopyFaceRegion(image, width, height, 0, i * faceSize, faceSize, faces[i]);
        }
    }
    else if (width % 4 == 0 && height % 3 == 0 && width / 4 == height / 3)
    {
        layoutName = "cross_4x3";
        faceSize = width / 4;
        std::array<std::pair<int, int>, 6> cells = {
            std::pair<int, int>{ 2, 1 }, // rt
            std::pair<int, int>{ 0, 1 }, // lf
            std::pair<int, int>{ 1, 0 }, // up
            std::pair<int, int>{ 1, 2 }, // dn
            std::pair<int, int>{ 3, 1 }, // bk
            std::pair<int, int>{ 1, 1 }  // ft
        };
        for (int i = 0; i < 6; ++i)
        {
            CopyFaceRegion(image, width, height, cells[i].first * faceSize, cells[i].second * faceSize, faceSize, faces[i]);
        }
    }
    else if (width % 3 == 0 && height % 4 == 0 && width / 3 == height / 4)
    {
        layoutName = "cross_3x4";
        faceSize = width / 3;
        std::array<std::pair<int, int>, 6> cells = {
            std::pair<int, int>{ 2, 1 }, // rt
            std::pair<int, int>{ 0, 1 }, // lf
            std::pair<int, int>{ 1, 0 }, // up
            std::pair<int, int>{ 1, 2 }, // dn
            std::pair<int, int>{ 1, 3 }, // bk
            std::pair<int, int>{ 1, 1 }  // ft
        };
        for (int i = 0; i < 6; ++i)
        {
            CopyFaceRegion(image, width, height, cells[i].first * faceSize, cells[i].second * faceSize, faceSize, faces[i]);
        }
    }
    else if (width == height * 2)
    {
        layoutName = "equirect";
        faceSize = std::clamp(height / 2, 256, 2048);
        for (int i = 0; i < 6; ++i)
        {
            RenderEquirectFace(image, width, height, i, faceSize, faces[i]);
        }
    }
    else
    {
        stbi_image_free(image);
        m_skyboxStatus = "Unsupported skybox image layout. Use 6x1, 1x6, 4x3/3x4 cross, or 2:1 equirectangular.";
        return false;
    }

    std::string baseName = std::filesystem::path(imagePath).stem().string();
    std::string outputFolder = ImportedSkyboxFolder(imagePath);
    SkyboxDefinition imported;
    imported.name = baseName + "_" + layoutName;
    imported.folder = outputFolder;

    bool written = WriteSkyboxFaces(outputFolder, imported.name, faces, faceSize, imported.faces);
    stbi_image_free(image);
    if (!written)
    {
        m_skyboxStatus = "Failed to write generated skybox faces";
        return false;
    }

    m_skyboxDefinition = imported;
    return ReloadSkybox();
}

bool pwg::EditorScene::ConsumeDirtyFlag()
{
    bool dirty = m_dirty;
    m_dirty = false;
    return dirty;
}

bool& pwg::EditorScene::ShowMainView()
{
    return m_showMainView;
}

bool& pwg::EditorScene::ShowTerrainControls()
{
    return m_showTerrainControls;
}

bool& pwg::EditorScene::ShowWorld()
{
    return m_showWorld;
}

bool& pwg::EditorScene::ShowOutliner()
{
    return m_showOutliner;
}
bool& pwg::EditorScene::ShowObjects()
{
    return m_showObjects;
}

std::shared_ptr<pwg::WorldObjectManager> pwg::EditorScene::GetWorldObjectManager()
{
    return m_worldObjectManager;
}
bool& pwg::EditorScene::ShowDebugWindow()
{
    return m_showDebugWindow;
}

bool& pwg::EditorScene::ShowMaterialLibrary()
{
    return m_showMaterialLibrary;
}

bool& pwg::EditorScene::ShowTerrainGraph()
{
    return m_showTerrainGraph;
}

const std::string& pwg::EditorScene::GetProjectLayoutIni() const
{
    return m_projectLayoutIni;
}

void pwg::EditorScene::SetProjectLayoutIni(const std::string& layoutIni)
{
    m_projectLayoutIni = layoutIni;
}

pwg::controls::TerrainGraphViewState& pwg::EditorScene::GetTerrainGraphViewState()
{
    return m_terrainGraphViewState;
}

const pwg::controls::TerrainGraphViewState& pwg::EditorScene::GetTerrainGraphViewState() const
{
    return m_terrainGraphViewState;
}

void pwg::EditorScene::SetTerrainGraphViewState(const controls::TerrainGraphViewState& viewState)
{
    m_terrainGraphViewState = viewState;
}

void pwg::EditorScene::ResetEditorWindows()
{
    m_showMainView = true;
    m_showOutliner = true;
    m_showTerrainControls = false;
    m_showWorld = true;
    m_showObjects = true;
    m_showDebugWindow = true;
    m_showMaterialLibrary = true;
    m_showTerrainGraph = true;
}

void pwg::EditorScene::DrawOutliner()
{
    if (!m_showOutliner)
    {
        return;
    }

    ImGui::Begin("Outliner", &m_showOutliner);
    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Main View", &m_showMainView);
        if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Terrain Graph", &m_showTerrainGraph);
            ImGui::Checkbox("Debug", &m_showDebugWindow);
            ImGui::TreePop();
        }
        ImGui::Checkbox("Material Library", &m_showMaterialLibrary);
        ImGui::Checkbox("World", &m_showWorld);
        ImGui::Checkbox("Objects", &m_showObjects);
        ImGui::TreePop();
    }
    ImGui::End();
}

pwg::ViewportPlacementInput pwg::EditorScene::GetViewportPlacementInput(const ImVec2& viewportMin, const ImVec2& viewportSize) const
{
    ViewportPlacementInput input;
    input.viewportMin = viewportMin;
    input.viewportSize = viewportSize;
    input.mousePos = ImGui::GetIO().MousePos;
    input.mouseInViewport = input.mousePos.x >= viewportMin.x && input.mousePos.x <= viewportMin.x + viewportSize.x &&
        input.mousePos.y >= viewportMin.y && input.mousePos.y <= viewportMin.y + viewportSize.y;
    input.leftClicked = input.mouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    input.rightClicked = input.mouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    input.escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
    return input;
}

void pwg::EditorScene::BeginObjectPlacement(int assetId)
{
    if (!m_worldObjectManager || !m_terrain)
    {
        return;
    }

    ModelAsset* asset = m_worldObjectManager->FindAsset(assetId);
    if (!asset || !asset->loaded)
    {
        m_worldObjectManager->SetStatus("Select a loaded asset before placing an object");
        return;
    }

    m_placementPreviewTransform = Transform{};
    m_placementPreviewTransform.position = { 0.0f, m_terrain->GetTerrainHeightAt(0.0f, 0.0f) - 4.0f, 0.0f };
    m_placementPreviewTransform.scale = asset->defaultScale;
    m_placementAssetId = assetId;
    m_placementPreviewValid = true;
    m_placementMouseArmed = false;
    m_objectPlacementActive = true;
    m_viewportInputCaptured = true;
    m_worldObjectManager->SelectObject(0);
    m_worldObjectManager->SetStatus("Placement: move cursor into viewport");
}

void pwg::EditorScene::UpdateObjectPlacement(const ViewportPlacementInput& input, std::shared_ptr<ICamera> camera)
{
    (void)camera;
    if (!m_objectPlacementActive || !m_worldObjectManager)
    {
        return;
    }

    if (input.rightClicked || input.escapePressed)
    {
        m_objectPlacementActive = false;
        m_placementAssetId = 0;
        m_placementPreviewValid = false;
        m_placementMouseArmed = false;
        m_worldObjectManager->SetStatus("Placement cancelled");
        return;
    }

    if (!input.mouseInViewport)
    {
        if (!m_placementMouseArmed)
        {
            m_worldObjectManager->SetStatus("Placement: move cursor into viewport");
        }
        return;
    }

    if (!m_placementMouseArmed)
    {
        m_placementMouseArmed = true;
        m_worldObjectManager->SetStatus("Placement: LMB place, RMB/Esc cancel");
    }

    if (m_terrain && camera && input.viewportSize.x > 1.0f && input.viewportSize.y > 1.0f)
    {
        constexpr float groundOffset = -4.0f;
        float worldHalfSize = (static_cast<float>(m_terrain->GetSize()) - 1.0f) * 0.5f;
        float u = glm::clamp((input.mousePos.x - input.viewportMin.x) / input.viewportSize.x, 0.0f, 1.0f);
        float v = glm::clamp((input.mousePos.y - input.viewportMin.y) / input.viewportSize.y, 0.0f, 1.0f);
        float screenX = (u - 0.5f) * 2.0f;
        float screenY = (0.5f - v) * 2.0f;

        glm::mat4 inverseView = glm::inverse(camera->GetViewMatrix());
        glm::vec3 cameraRight = glm::vec3(inverseView[0]);
        glm::vec3 cameraForward = -glm::vec3(inverseView[2]);
        glm::vec3 planarRight(cameraRight.x, 0.0f, cameraRight.z);
        glm::vec3 planarForward(cameraForward.x, 0.0f, cameraForward.z);
        if (glm::length(planarRight) < 0.0001f)
        {
            planarRight = { 1.0f, 0.0f, 0.0f };
        }
        if (glm::length(planarForward) < 0.0001f)
        {
            planarForward = { 0.0f, 0.0f, -1.0f };
        }
        planarRight = glm::normalize(planarRight);
        planarForward = glm::normalize(planarForward);

        glm::vec3 terrainPosition = planarRight * (screenX * worldHalfSize) + planarForward * (screenY * worldHalfSize);
        terrainPosition.x = glm::clamp(terrainPosition.x, -worldHalfSize, worldHalfSize);
        terrainPosition.z = glm::clamp(terrainPosition.z, -worldHalfSize, worldHalfSize);
        terrainPosition.y = m_terrain->GetTerrainHeightAt(terrainPosition.x, terrainPosition.z) + groundOffset;
        m_placementPreviewTransform.position = terrainPosition;
        WorldObject groundedPreview;
        groundedPreview.assetId = m_placementAssetId;
        groundedPreview.transform = m_placementPreviewTransform;
        m_worldObjectManager->GroundObjectToTerrain(groundedPreview, m_terrain);
        m_placementPreviewTransform = groundedPreview.transform;
        m_placementPreviewValid = true;
    }

    if (input.leftClicked && m_placementPreviewValid)
    {
        WorldObject* object = m_worldObjectManager->CreateObject(m_placementAssetId, m_placementPreviewTransform.position);
        if (object)
        {
            object->transform = m_placementPreviewTransform;
            object->snapToTerrain = true;
            object->freeTransform = false;
            m_worldObjectManager->GroundObjectToTerrain(*object, m_terrain);
            m_dirty = true;
        }
        m_objectPlacementActive = false;
        m_placementAssetId = 0;
        m_placementPreviewValid = false;
        m_placementMouseArmed = false;
    }
}

void pwg::EditorScene::BeginAssetImport(int assetId, bool newlyImported)
{
    ModelAsset* asset = m_worldObjectManager ? m_worldObjectManager->FindAsset(assetId) : nullptr;
    if (!asset)
    {
        return;
    }
    m_assetImportId = assetId;
    m_assetImportOriginal = *asset;
    m_assetImportDraft = *asset;
    m_assetImportNew = newlyImported;
    m_assetImportDirty = false;
    m_assetImportCloseRequested = false;
    m_assetImportActive = true;
    m_objectPlacementActive = false;
    m_cameraManager->SetCamera(CameraType::EDITOR);
    m_cameraManager->SetDefaultCameraPosition(300);
}

void pwg::EditorScene::ApplyAssetImport()
{
    ModelAsset* asset = m_worldObjectManager ? m_worldObjectManager->FindAsset(m_assetImportId) : nullptr;
    if (!asset)
    {
        CloseAssetImport();
        return;
    }
    asset->importRotation = m_assetImportDraft.importRotation;
    asset->defaultScale = m_assetImportDraft.defaultScale;
    asset->pivotOffset = m_assetImportDraft.pivotOffset;
    asset->defaultSurfaceMode = m_assetImportDraft.defaultSurfaceMode;
    m_worldObjectManager->RefreshAssetInstances(asset->id, m_terrain);
    m_dirty = true;
    CloseAssetImport();
}

void pwg::EditorScene::CloseAssetImport()
{
    m_assetImportActive = false;
    m_assetImportDirty = false;
    m_assetImportNew = false;
    m_assetImportCloseRequested = false;
    m_assetImportId = 0;
}

void pwg::EditorScene::DrawAssetImportWorkspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    float settingsWidth = std::clamp(workSize.x * 0.28f, 340.0f, 480.0f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking;

    ImGui::SetNextWindowPos(workPos);
    ImGui::SetNextWindowSize(ImVec2(workSize.x - settingsWidth, workSize.y));
    ImGui::Begin("Asset Preview", nullptr, flags);
    ImVec2 previewSize = ImGui::GetContentRegionAvail();
    int width = std::max(1, static_cast<int>(previewSize.x));
    int height = std::max(1, static_cast<int>(previewSize.y));
    if (m_frameBuffer->GetWidth() != width || m_frameBuffer->GetHeight() != height)
    {
        m_frameBuffer->Resize(width, height);
        m_hdrFrameBuffer->Resize(width, height);
    }
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    auto camera = m_cameraManager->GetActiveCamera();
    if (camera)
    {
        camera->SetAspectRatio(m_aspectRatio);
    }

    EnvironmentLighting lighting = m_environment->GetLighting();
    lighting.shadowsEnabled = false;
    m_hdrFrameBuffer->Bind();
    m_renderer.BeginFrame();
    m_renderer.SetCamera(camera);
    m_renderer.SetEnvironmentLighting(lighting);
    m_renderer.SetShadowMaps({}, {}, {}, false);
    if (m_worldObjectManager)
    {
        m_worldObjectManager->DrawAssetPreview(m_renderer, m_assetImportDraft);
    }
    m_renderer.EndFrame();
    m_hdrFrameBuffer->Unbind();
    m_frameBuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    m_postProcessor->Render(m_hdrFrameBuffer->GetTextureID(), m_environment->GetSettings(), width, height);
    m_frameBuffer->Unbind();

    ImGui::Image((ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(), previewSize, ImVec2(0, 1), ImVec2(1, 0));
    ImVec2 imageMin = ImGui::GetItemRectMin();
    ImVec2 imageMax = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (int i = 1; i < 12; ++i)
    {
        float t = static_cast<float>(i) / 12.0f;
        drawList->AddLine(ImVec2(glm::mix(imageMin.x, imageMax.x, t), imageMin.y),
            ImVec2(glm::mix(imageMin.x, imageMax.x, t), imageMax.y), IM_COL32(100, 120, 135, 28));
        drawList->AddLine(ImVec2(imageMin.x, glm::mix(imageMin.y, imageMax.y, t)),
            ImVec2(imageMax.x, glm::mix(imageMin.y, imageMax.y, t)), IM_COL32(100, 120, 135, 28));
    }
    drawList->AddLine(ImVec2(imageMin.x, (imageMin.y + imageMax.y) * 0.5f),
        ImVec2(imageMax.x, (imageMin.y + imageMax.y) * 0.5f), IM_COL32(90, 190, 120, 95), 2.0f);

    if (m_assetImportShowBounds && camera)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, m_assetImportDraft.pivotOffset);
        model = glm::rotate(model, m_assetImportDraft.importRotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, m_assetImportDraft.importRotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, m_assetImportDraft.importRotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, m_assetImportDraft.defaultScale);
        glm::mat4 viewProjection = camera->GetProjectionMatrix() * camera->GetViewMatrix();
        ImVec2 points[8];
        bool visible[8]{};
        for (int corner = 0; corner < 8; ++corner)
        {
            glm::vec3 local((corner & 1) ? m_assetImportDraft.boundsMax.x : m_assetImportDraft.boundsMin.x,
                (corner & 2) ? m_assetImportDraft.boundsMax.y : m_assetImportDraft.boundsMin.y,
                (corner & 4) ? m_assetImportDraft.boundsMax.z : m_assetImportDraft.boundsMin.z);
            glm::vec4 clip = viewProjection * model * glm::vec4(local, 1.0f);
            visible[corner] = clip.w > 0.001f;
            if (visible[corner])
            {
                glm::vec3 ndc = glm::vec3(clip) / clip.w;
                points[corner] = ImVec2(imageMin.x + (ndc.x * 0.5f + 0.5f) * previewSize.x,
                    imageMin.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * previewSize.y);
            }
        }
        constexpr int edges[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
        for (const auto& edge : edges)
        {
            if (visible[edge[0]] && visible[edge[1]])
            {
                drawList->AddLine(points[edge[0]], points[edge[1]], IM_COL32(255, 190, 65, 210), 1.5f);
            }
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(workPos.x + workSize.x - settingsWidth, workPos.y));
    ImGui::SetNextWindowSize(ImVec2(settingsWidth, workSize.y));
    ImGui::Begin("Asset Import Settings", nullptr, flags);
    ImGui::TextUnformatted(m_assetImportDraft.name.c_str());
    ImGui::TextWrapped("%s", m_assetImportDraft.path.c_str());
    ImGui::Separator();

    bool changed = false;
    glm::vec3 degrees = glm::degrees(m_assetImportDraft.importRotation);
    if (ImGui::DragFloat3("Import Rotation", &degrees.x, 0.5f, -360.0f, 360.0f, "%.1f deg"))
    {
        m_assetImportDraft.importRotation = glm::radians(degrees);
        changed = true;
    }
    const struct { const char* label; int axis; float amount; } presets[] = {
        { "X +90", 0, 90.0f }, { "X -90", 0, -90.0f }, { "Y +90", 1, 90.0f },
        { "Y -90", 1, -90.0f }, { "Z +90", 2, 90.0f }, { "Z -90", 2, -90.0f }
    };
    for (int i = 0; i < 6; ++i)
    {
        if (i > 0 && i % 2 != 0) ImGui::SameLine();
        if (ImGui::Button(presets[i].label))
        {
            m_assetImportDraft.importRotation[presets[i].axis] += glm::radians(presets[i].amount);
            changed = true;
        }
    }
    if (ImGui::Button("Reset Rotation"))
    {
        m_assetImportDraft.importRotation = glm::vec3(0.0f);
        changed = true;
    }

    float uniformScale = m_assetImportDraft.defaultScale.x;
    if (ImGui::DragFloat("Uniform Scale", &uniformScale, 0.01f, 0.001f, 1000.0f))
    {
        uniformScale = std::max(uniformScale, 0.001f);
        m_assetImportDraft.defaultScale = glm::vec3(uniformScale);
        changed = true;
    }
    changed = ImGui::DragFloat3("Pivot", &m_assetImportDraft.pivotOffset.x, 0.1f) || changed;

    auto orientedBounds = [&]()
    {
        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, m_assetImportDraft.importRotation.x, glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, m_assetImportDraft.importRotation.y, glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, m_assetImportDraft.importRotation.z, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, m_assetImportDraft.defaultScale);
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (int corner = 0; corner < 8; ++corner)
        {
            glm::vec3 local((corner & 1) ? m_assetImportDraft.boundsMax.x : m_assetImportDraft.boundsMin.x,
                (corner & 2) ? m_assetImportDraft.boundsMax.y : m_assetImportDraft.boundsMin.y,
                (corner & 4) ? m_assetImportDraft.boundsMax.z : m_assetImportDraft.boundsMin.z);
            glm::vec3 point = glm::vec3(transform * glm::vec4(local, 1.0f));
            minimum = glm::min(minimum, point);
            maximum = glm::max(maximum, point);
        }
        return std::pair<glm::vec3, glm::vec3>(minimum, maximum);
    };
    if (ImGui::Button("Center XZ"))
    {
        auto [minimum, maximum] = orientedBounds();
        m_assetImportDraft.pivotOffset.x = -(minimum.x + maximum.x) * 0.5f;
        m_assetImportDraft.pivotOffset.z = -(minimum.z + maximum.z) * 0.5f;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Place Bottom On Ground"))
    {
        auto [minimum, maximum] = orientedBounds();
        m_assetImportDraft.pivotOffset.y = -minimum.y;
        changed = true;
    }

    int surfaceMode = static_cast<int>(m_assetImportDraft.defaultSurfaceMode);
    const char* surfaceLabels[] = { "Snap to Terrain", "Free Transform", "Align to Normal" };
    if (ImGui::Combo("Surface Mode", &surfaceMode, surfaceLabels, 3))
    {
        m_assetImportDraft.defaultSurfaceMode = static_cast<ObjectSurfaceMode>(surfaceMode);
        changed = true;
    }
    changed = ImGui::Checkbox("Show Bounds", &m_assetImportShowBounds) || changed;
    m_assetImportDirty = m_assetImportDirty || changed;

    ImGui::Separator();
    if (ImGui::Button("Apply", ImVec2(90, 0)))
    {
        ApplyAssetImport();
        ImGui::End();
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(90, 0)))
    {
        CloseAssetImport();
        ImGui::End();
        return;
    }
    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(90, 0)))
    {
        if (m_assetImportDirty) m_assetImportCloseRequested = true;
        else CloseAssetImport();
    }
    if (m_assetImportCloseRequested)
    {
        ImGui::OpenPopup("Unsaved Asset Changes");
        m_assetImportCloseRequested = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Asset Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Apply changes before returning to the world?");
        if (ImGui::Button("Apply"))
        {
            ApplyAssetImport();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard"))
        {
            CloseAssetImport();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stay"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
void pwg::EditorScene::DrawObjectsWindow()
{
    if (!m_showObjects || !m_worldObjectManager)
    {
        return;
    }

    ImGui::Begin("Objects", &m_showObjects);
    ImGui::TextWrapped("Status: %s", m_worldObjectManager->GetStatus().c_str());
    if (m_objectPlacementActive)
    {
        ImGui::TextUnformatted(m_placementMouseArmed ? "Placement: LMB place, RMB/Esc cancel" : "Placement: move cursor into viewport");
    }

    if (ImGui::Button("Import Model..."))
    {
        std::string path = platform::OpenObjDialog();
        if (!path.empty())
        {
            if (auto* asset = m_worldObjectManager->ImportObj(path))
            {
                BeginAssetImport(asset->id, true);
                m_dirty = true;
            }
        }
    }

    ImGui::SeparatorText("Assets");
    for (const auto& asset : m_worldObjectManager->GetAssets())
    {
        ImGui::PushID(asset.id);
        bool selected = asset.id == m_worldObjectManager->GetSelectedAssetId();
        std::string label = asset.name + (asset.loaded ? "" : " (missing)");
        if (!asset.submeshes.empty())
        {
            label += " [" + std::to_string(asset.submeshes.size()) + " parts]";
        }
        if (ImGui::SmallButton("Edit"))
        {
            BeginAssetImport(asset.id, false);
        }
        ImGui::SameLine();
        if (ImGui::Selectable(label.c_str(), selected))
        {
            m_worldObjectManager->SelectAsset(asset.id);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(asset.path.c_str());
            if (!asset.submeshes.empty())
            {
                ImGui::Separator();
                for (const auto& submesh : asset.submeshes)
                {
                    ImGui::Text("%s", submesh.name.c_str());
                    if (!submesh.albedoPath.empty())
                    {
                        ImGui::TextDisabled("  %s", submesh.albedoPath.c_str());
                    }
                }
            }
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Procedural Rule Sets");
    if (ImGui::Button("New Rule Set"))
    {
        if (auto* created = m_worldObjectManager->CreateRuleSet())
        {
            m_selectedRuleSetId = created->id;
            m_dirty = true;
        }
    }

    for (const auto& ruleSet : m_worldObjectManager->GetRuleSets())
    {
        ImGui::PushID(ruleSet.id);
        int generatedCount = static_cast<int>(std::count_if(m_worldObjectManager->GetObjects().begin(), m_worldObjectManager->GetObjects().end(), [&](const WorldObject& object)
        {
            return object.generated && object.sourceRuleSetId == ruleSet.id;
        }));
        std::string label = ruleSet.name + " (" + std::to_string(generatedCount) + ")";
        if (ImGui::Selectable(label.c_str(), m_selectedRuleSetId == ruleSet.id))
        {
            m_selectedRuleSetId = ruleSet.id;
        }
        ImGui::PopID();
    }

    if (auto* ruleSet = m_worldObjectManager->FindRuleSet(m_selectedRuleSetId))
    {
        ImGui::PushID(ruleSet->id);
        char ruleName[128] = {};
        std::snprintf(ruleName, sizeof(ruleName), "%s", ruleSet->name.c_str());
        bool changed = false;
        if (ImGui::InputText("Name", ruleName, sizeof(ruleName)))
        {
            ruleSet->name = ruleName;
            changed = true;
        }
        changed = ImGui::Checkbox("Enabled", &ruleSet->enabled) || changed;
        changed = ImGui::InputInt("Seed", &ruleSet->seed) || changed;
        changed = ImGui::SliderFloat("Density", &ruleSet->density, 0.0f, 1.0f) || changed;
        changed = ImGui::DragFloat("Minimum spacing", &ruleSet->spacing, 1.0f, 4.0f, 500.0f) || changed;
        changed = ImGui::DragFloatRange2("Height", &ruleSet->minHeight, &ruleSet->maxHeight, 1.0f, -20000.0f, 20000.0f) || changed;
        changed = ImGui::DragFloatRange2("Slope (degrees)", &ruleSet->minSlopeDegrees, &ruleSet->maxSlopeDegrees, 0.5f, 0.0f, 90.0f) || changed;
        std::vector<std::string> placementMasks;
        for (const auto& [name, mask] : m_terrain->GetChunkManager()->GetMasks())
        {
            if (mask && mask->texture && name != "NormalMask" && name != "WaterFlux" && name != "WaterFluxDiagonal")
            {
                placementMasks.push_back(name);
            }
        }
        std::sort(placementMasks.begin(), placementMasks.end());
        const char* maskPreview = ruleSet->maskName.empty() ? "None" : ruleSet->maskName.c_str();
        if (ImGui::BeginCombo("Placement mask", maskPreview))
        {
            if (ImGui::Selectable("None", ruleSet->maskName.empty()))
            {
                ruleSet->maskName.clear();
                changed = true;
            }
            for (const auto& maskName : placementMasks)
            {
                if (ImGui::Selectable(maskName.c_str(), ruleSet->maskName == maskName))
                {
                    ruleSet->maskName = maskName;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        if (!ruleSet->maskName.empty())
        {
            changed = ImGui::DragFloatRange2("Mask value", &ruleSet->minMaskValue, &ruleSet->maxMaskValue, 0.01f, -10000.0f, 10000.0f) || changed;
        }
        changed = ImGui::DragFloat("Avoid water distance", &ruleSet->avoidWaterDistance, 1.0f, 0.0f, 1000.0f) || changed;
        if (ruleSet->avoidWaterDistance > 0.0f)
        {
            changed = ImGui::DragFloat("Water threshold", &ruleSet->waterThreshold, 0.001f, 0.0f, 1000.0f) || changed;
        }
        changed = ImGui::DragFloat("Avoid river distance", &ruleSet->avoidRiverDistance, 1.0f, 0.0f, 1000.0f) || changed;
        if (ruleSet->avoidRiverDistance > 0.0f)
        {
            changed = ImGui::DragFloat("River threshold", &ruleSet->riverThreshold, 0.001f, 0.0f, 1000.0f) || changed;
        }
        changed = ImGui::DragFloatRange2("Scale multiplier", &ruleSet->minScale, &ruleSet->maxScale, 0.01f, 0.01f, 10.0f) || changed;
        changed = ImGui::DragFloatRange2("Rotation X (deg)", &ruleSet->minRotationDegrees.x, &ruleSet->maxRotationDegrees.x, 1.0f, -360.0f, 360.0f) || changed;
        changed = ImGui::DragFloatRange2("Rotation Y (deg)", &ruleSet->minRotationDegrees.y, &ruleSet->maxRotationDegrees.y, 1.0f, -360.0f, 360.0f) || changed;
        changed = ImGui::DragFloatRange2("Rotation Z (deg)", &ruleSet->minRotationDegrees.z, &ruleSet->maxRotationDegrees.z, 1.0f, -360.0f, 360.0f) || changed;
        changed = ImGui::Checkbox("Align to normal", &ruleSet->alignToNormal) || changed;

        ImGui::TextUnformatted("Assets and weights");
        for (const auto& asset : m_worldObjectManager->GetAssets())
        {
            ImGui::PushID(asset.id);
            auto findEntry = [&]()
            {
                return std::find_if(ruleSet->assets.begin(), ruleSet->assets.end(), [&](const ObjectRuleAsset& entry) { return entry.assetId == asset.id; });
            };
            bool included = findEntry() != ruleSet->assets.end();
            if (ImGui::Checkbox("##included", &included))
            {
                if (included)
                {
                    ruleSet->assets.push_back({ asset.id, 1.0f });
                }
                else
                {
                    ruleSet->assets.erase(std::remove_if(ruleSet->assets.begin(), ruleSet->assets.end(), [&](const ObjectRuleAsset& entry) { return entry.assetId == asset.id; }), ruleSet->assets.end());
                }
                changed = true;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(asset.name.c_str());
            auto entry = findEntry();
            if (entry != ruleSet->assets.end())
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                changed = ImGui::DragFloat("Weight", &entry->weight, 0.05f, 0.0f, 100.0f) || changed;
            }
            ImGui::PopID();
        }

        ImGui::SeparatorText("Exclusion Zones");
        if (ImGui::Button("Add Exclusion Zone"))
        {
            if (auto* zone = m_worldObjectManager->CreateExclusionZone(ruleSet->id))
            {
                m_selectedExclusionZoneId = zone->id;
                changed = true;
            }
        }
        for (const auto& zone : m_worldObjectManager->GetExclusionZones())
        {
            if (zone.ruleSetId != 0 && zone.ruleSetId != ruleSet->id)
            {
                continue;
            }
            ImGui::PushID(zone.id);
            std::string zoneLabel = zone.name + (zone.ruleSetId == 0 ? " (Global)" : "");
            if (ImGui::Selectable(zoneLabel.c_str(), m_selectedExclusionZoneId == zone.id))
            {
                m_selectedExclusionZoneId = zone.id;
            }
            ImGui::PopID();
        }
        if (auto* zone = m_worldObjectManager->FindExclusionZone(m_selectedExclusionZoneId);
            zone && (zone->ruleSetId == 0 || zone->ruleSetId == ruleSet->id))
        {
            ImGui::PushID(zone->id);
            char zoneNameBuffer[128] = {};
            std::snprintf(zoneNameBuffer, sizeof(zoneNameBuffer), "%s", zone->name.c_str());
            if (ImGui::InputText("Zone Name", zoneNameBuffer, sizeof(zoneNameBuffer)))
            {
                zone->name = zoneNameBuffer;
                changed = true;
            }
            changed = ImGui::Checkbox("Zone Enabled", &zone->enabled) || changed;
            bool global = zone->ruleSetId == 0;
            if (ImGui::Checkbox("Global Zone", &global))
            {
                zone->ruleSetId = global ? 0 : ruleSet->id;
                changed = true;
            }
            int shape = static_cast<int>(zone->shape);
            const char* shapeLabels[] = { "Circle", "Box" };
            if (ImGui::Combo("Shape", &shape, shapeLabels, 2))
            {
                zone->shape = static_cast<ObjectExclusionShape>(shape);
                changed = true;
            }
            changed = ImGui::DragFloat2("Center XZ", &zone->center.x, 0.5f) || changed;
            if (zone->shape == ObjectExclusionShape::Circle)
            {
                changed = ImGui::DragFloat("Radius", &zone->radius, 0.5f, 0.0f, 100000.0f) || changed;
            }
            else
            {
                changed = ImGui::DragFloat2("Half Extents XZ", &zone->halfExtents.x, 0.5f, 0.0f, 100000.0f) || changed;
            }
            if (ImGui::Button("Delete Exclusion Zone"))
            {
                int removedZoneId = zone->id;
                m_selectedExclusionZoneId = 0;
                m_worldObjectManager->RemoveExclusionZone(removedZoneId);
                changed = true;
            }
            ImGui::PopID();
        }
        int chunkCount = m_terrain->GetChunkManager()->GetChunkCount();
        m_objectChunkX = std::clamp(m_objectChunkX, 0, chunkCount - 1);
        m_objectChunkZ = std::clamp(m_objectChunkZ, 0, chunkCount - 1);
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("Chunk X", &m_objectChunkX);
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("Chunk Z", &m_objectChunkZ);
        m_objectChunkX = std::clamp(m_objectChunkX, 0, chunkCount - 1);
        m_objectChunkZ = std::clamp(m_objectChunkZ, 0, chunkCount - 1);
        if (ImGui::Button("Regenerate Chunk"))
        {
            m_worldObjectManager->GenerateRuleSet(ruleSet->id, m_terrain, m_objectChunkX, m_objectChunkZ);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Chunk"))
        {
            m_worldObjectManager->ClearGenerated(ruleSet->id, m_objectChunkX, m_objectChunkZ);
            changed = true;
        }

        if (ImGui::Button("Generate"))
        {
            m_worldObjectManager->GenerateRuleSet(ruleSet->id, m_terrain);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Generated"))
        {
            m_worldObjectManager->ClearGenerated(ruleSet->id);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Bake"))
        {
            m_worldObjectManager->BakeGenerated(ruleSet->id);
            changed = true;
        }
        if (ImGui::Button("Delete Rule Set"))
        {
            int removedId = ruleSet->id;
            m_selectedRuleSetId = 0;
    m_selectedExclusionZoneId = 0;
            m_worldObjectManager->RemoveRuleSet(removedId);
            changed = true;
        }
        if (changed)
        {
            m_dirty = true;
        }
        ImGui::PopID();
    }

    if (ImGui::Button("Place"))
    {
        BeginObjectPlacement(m_worldObjectManager->GetSelectedAssetId());
    }
    ImGui::SameLine();
    if (ImGui::Button("Duplicate"))
    {
        if (m_worldObjectManager->DuplicateSelected())
        {
            m_dirty = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
    {
        m_worldObjectManager->DeleteSelected();
        m_dirty = true;
    }

    ImGui::SeparatorText("Instances");
    for (const auto& object : m_worldObjectManager->GetObjects())
    {
        ImGui::PushID(object.id);
        if (ImGui::Selectable(object.name.c_str(), object.selected))
        {
            m_worldObjectManager->SelectObject(object.id);
        }
        ImGui::PopID();
    }

    if (auto* object = m_worldObjectManager->GetSelectedObject())
    {
        ImGui::SeparatorText("Transform");
        char nameBuffer[128] = {};
        std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", object->name.c_str());
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
        {
            object->name = nameBuffer;
            m_dirty = true;
        }

        bool changed = false;
        changed = ImGui::Checkbox("Visible", &object->visible) || changed;
        changed = ImGui::Checkbox("Snap to terrain", &object->snapToTerrain) || changed;
        ImGui::SameLine();
        changed = ImGui::Checkbox("Free transform", &object->freeTransform) || changed;
        changed = ImGui::Checkbox("Align to normal (reserved)", &object->alignToNormal) || changed;
        changed = ImGui::DragFloat3("Position", &object->transform.position.x, 0.25f) || changed;
        changed = ImGui::DragFloat3("Rotation", &object->transform.rotation.x, 0.01f) || changed;
        changed = ImGui::DragFloat3("Scale", &object->transform.scale.x, 0.01f, 0.001f, 100.0f) || changed;
        if (changed)
        {
            if (object->snapToTerrain && !object->freeTransform)
            {
                m_worldObjectManager->GroundObjectToTerrain(*object, m_terrain);
            }
            m_dirty = true;
        }
    }

    ImGui::End();
}

void pwg::EditorScene::DrawWorldWindow()
{
    if (!m_showWorld)
    {
        return;
    }

    ImGui::Begin("World", &m_showWorld);
    if (ImGui::BeginTabBar("WorldTabs"))
    {
        if (ImGui::BeginTabItem("Sky"))
        {
            DrawSkyboxControls();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Environment"))
        {
            bool changed = controls::EnvironmentControls::ShowControl(m_environment);
            if (changed || (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Left)))
            {
                m_dirty = true;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void pwg::EditorScene::DrawSkyboxControls()
{
    ImGui::Text("Skybox");
    ImGui::Text("Active: %s", m_skyboxDefinition.name.c_str());
    ImGui::TextWrapped("Status: %s", m_skyboxStatus.c_str());

    std::vector<SkyboxDefinition> presets = FindSkyboxDefinitions("../assets/textures/skybox");
    if (ImGui::BeginCombo("Preset", m_skyboxDefinition.name.c_str()))
    {
        for (const auto& preset : presets)
        {
            bool selected = preset.name == m_skyboxDefinition.name && preset.folder == m_skyboxDefinition.folder;
            if (ImGui::Selectable(preset.name.c_str(), selected))
            {
                m_skyboxDefinition = preset;
                if (ReloadSkybox())
                {
                    m_dirty = true;
                }
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Load Skybox Folder..."))
    {
        std::string folder = platform::PickFolderDialog("Select skybox folder");
        if (!folder.empty() && LoadSkyboxFolder(folder))
        {
            m_dirty = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Single Image..."))
    {
        std::string imagePath = platform::OpenImageDialog();
        if (!imagePath.empty() && ImportSkyboxImage(imagePath))
        {
            m_dirty = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Skybox"))
    {
        if (ReloadSkybox())
        {
            m_dirty = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Skybox"))
    {
        ResetSkyboxToDefault();
        m_dirty = true;
    }

    if (ImGui::TreeNode("Faces"))
    {
        auto suffixes = SkyboxFaceSuffixes();
        for (int i = 0; i < 6; ++i)
        {
            ImGui::TextWrapped("%s: %s", suffixes[i], m_skyboxDefinition.faces[i].c_str());
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Face Orientation"))
    {
        auto suffixes = SkyboxFaceSuffixes();
        const char* rotationLabels[] = { "0", "90", "180", "270" };
        bool orientationChanged = false;
        for (int i = 0; i < 6; ++i)
        {
            ImGui::PushID(i);
            int rotation = ((m_skyboxDefinition.rotations[i] % 4) + 4) % 4;
            ImGui::Text("%s", suffixes[i]);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::Combo("Rotation", &rotation, rotationLabels, 4))
            {
                m_skyboxDefinition.rotations[i] = rotation;
                orientationChanged = true;
            }
            ImGui::SameLine();
            orientationChanged = ImGui::Checkbox("Flip H", &m_skyboxDefinition.flipHorizontal[i]) || orientationChanged;
            ImGui::SameLine();
            orientationChanged = ImGui::Checkbox("Flip V", &m_skyboxDefinition.flipVertical[i]) || orientationChanged;
            ImGui::PopID();
        }

        if (ImGui::Button("Reset Face Orientation"))
        {
            m_skyboxDefinition.rotations = {};
            m_skyboxDefinition.flipHorizontal = {};
            m_skyboxDefinition.flipVertical = {};
            orientationChanged = true;
        }

        if (orientationChanged)
        {
            if (ReloadSkybox())
            {
                m_dirty = true;
            }
        }
        ImGui::TreePop();
    }
}

bool pwg::EditorScene::LoadSkyboxFolder(const std::string& folder)
{
    auto definitions = FindSkyboxDefinitions(folder);
    if (definitions.empty())
    {
        m_skyboxStatus = "Folder does not contain a complete skybox set (_rt, _lf, _up, _dn, _bk, _ft)";
        return false;
    }

    m_skyboxDefinition = definitions.front();
    return ReloadSkybox();
}

void pwg::EditorScene::ResetSkyboxToDefault()
{
    m_skyboxDefinition = DefaultSkyboxDefinition();
    ReloadSkybox();
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









