#include <glad/glad.h>
#include "imgui.h"
#include "editorScene.h"
#include "controls/noiseControls.h"
#include "controls/terrainGraphEditor.h"
#include "controls/environmentControls.h"
#include "platform/nativeDialogs.h"
#include "debug/debugView.h"

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
	/** @brief Frame description for a Debug View screenshot (D-07): colour texture, camera pose, debug view. */
	pwg::debug::ScreenshotSource MakeScreenshotSource(pwg::CameraManager& cameras, pwg::Terrain& terrain,
		unsigned int texture, int width, int height)
	{
		pwg::debug::ScreenshotSource source;
		source.texture = texture;
		source.width = width;
		source.height = height;

		std::shared_ptr<pwg::ICamera> camera = cameras.GetActiveCamera();
		const pwg::CameraProperties* properties = nullptr;
		const char* name = "camera";
		switch (cameras.GetActiveCameraType())
		{
		case pwg::CameraType::EDITOR: name = "editor"; properties = &cameras.GetEditorCamera()->GetCameraProperties(); break;
		case pwg::CameraType::FLYOVER: name = "flyover"; properties = &cameras.GetFlyoverCamera()->GetCameraProperties(); break;
		case pwg::CameraType::PLAYER: name = "player"; properties = &cameras.GetPlayerCamera()->GetCameraProperties(); break;
		}
		if (camera && properties)
		{
			source.camera = pwg::debug::MakeCameraPose(name, camera->GetViewMatrix(), camera->GetProjectionMatrix(),
				properties->fov, properties->nearPlane, properties->farPlane, properties->aspectRatio);
		}
		else
		{
			source.camera.camera = name;
		}

		if (std::shared_ptr<pwg::TerrainDebug> debug = terrain.GetTerrainDebug())
		{
			source.wireframe = debug->GetDebugSettings().wireframe;
			const std::vector<pwg::DebugEntry>& entries = debug->GetEntries();
			const int selected = debug->GetDebugMode();
			if (debug->GetStatus() && selected >= 0 && static_cast<size_t>(selected) < entries.size())
			{
				source.debugMode = entries[static_cast<size_t>(selected)].name;
			}
		}
		return source;
	}

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
    std::string vegetationShadersPath = "../assets/shaders/vegetation/";
	std::string waterShadersPath = "../assets/shaders/water/";

    shaderManager->Load("terrainShader", terrainShadersPath + "terrain.vert", terrainShadersPath + "terrain.frag");
    shaderManager->Load("skyboxShader", skyboxShadersPath + "skybox.vert", skyboxShadersPath + "skybox.frag");
    shaderManager->Load("postprocessShader", postprocessShadersPath + "postprocess.vert", postprocessShadersPath + "postprocess.frag");
    shaderManager->Load("terrainShadowShader", shadowShadersPath + "terrain_shadow.vert", shadowShadersPath + "terrain_shadow.frag");
    shaderManager->Load("vegetationShader", vegetationShadersPath + "vegetation.vert", vegetationShadersPath + "vegetation.frag");
    shaderManager->Load("vegetationShadowShader", vegetationShadersPath + "vegetation_shadow.vert", vegetationShadersPath + "vegetation_shadow.frag");
	shaderManager->Load("waterShader", waterShadersPath + "water.vert", waterShadersPath + "water.frag");

    auto terrainShader = shaderManager->GetShader<pwg::Shader>("terrainShader");
    auto skyboxShader = shaderManager->GetShader<pwg::Shader>("skyboxShader");
    auto postprocessShader = shaderManager->GetShader<pwg::Shader>("postprocessShader");
    m_terrainShadowShader = shaderManager->GetShader<pwg::Shader>("terrainShadowShader");
    auto vegetationShader = shaderManager->GetShader<pwg::Shader>("vegetationShader");
    auto vegetationShadowShader = shaderManager->GetShader<pwg::Shader>("vegetationShadowShader");
	auto waterShader = shaderManager->GetShader<pwg::Shader>("waterShader");

    /*== TEXTURE LOADING ==*/
    std::string terrainTexturesPath = "../assets/textures/";
	textureManager->Load("WaterNormalLarge", terrainTexturesPath + "water/water_large_normal.jpg", TextureType::NORMAL);
	textureManager->Load("WaterNormalDetail", terrainTexturesPath + "water/water_detail_normal.jpg", TextureType::NORMAL);
    m_skyboxDefinition = DefaultSkyboxDefinition();
    textureManager->LoadCubeMapTexture("Skybox", FaceVector(m_skyboxDefinition), TextureType::ALBEDO);


    /*== CREATING MATERIALS ==*/
    auto terrainMaterial = materialManager->CreateMaterial("TerrainMaterial", terrainShader);
	auto waterMaterial = materialManager->CreateMaterial("WaterMaterial", waterShader);
	waterMaterial->SetUniformTexture("u_WaterNormalLarge", textureManager->GetTexture("WaterNormalLarge"));
	waterMaterial->SetUniformTexture("u_WaterNormalDetail", textureManager->GetTexture("WaterNormalDetail"));
	waterMaterial->SetCubeMapTexture("u_skybox", textureManager->GetCubeMapTexture("Skybox"));
    auto skyboxMaterial = materialManager->CreateMaterial("SkyboxMaterial", skyboxShader);

    m_terrainMaterialLibrary = std::make_shared<TerrainMaterialLibrary>(m_resourceManager, terrainMaterial);
    auto skybox = textureManager->GetCubeMapTexture("Skybox");

	m_terrain = std::make_shared<Terrain>(m_resourceManager);
	m_vegetationAssetRegistry = std::make_shared<VegetationAssetRegistry>();
    m_vegetationSystem = std::make_shared<VegetationSystem>(vegetationShader, vegetationShadowShader, m_vegetationAssetRegistry);
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
		return m_vegetationSystem ? m_vegetationSystem->GetCollisionGroundHeight(m_terrain, x, z) : m_terrain->GetTerrainHeightAt(x, z);
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
    if (m_vegetationSystem)
    {
        auto chunkManager = m_terrain->GetChunkManager();
        auto pipeline = chunkManager->GetTerrainComputePipeline();
        if (pipeline)
        {
            m_vegetationSystem->Sync(m_terrain, *pipeline, chunkManager->GetGenerationRevision());
        }
        m_vegetationSystem->Update(m_terrain, activeCamera, dt);
    }
    m_environment->Update(dt);

}

void pwg::EditorScene::Draw()
{
    m_viewportInputCaptured = false;

    if (!m_mainViewFullscreen)
    {
        DrawOutliner();
    }
    else
    {
        m_terrainGraphInputCaptured = false;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    m_showMainView = true;
    {
    const ImGuiWindowFlags mainViewFlags = m_mainViewFullscreen
        ? ImGuiWindowFlags_NoDecoration |
          ImGuiWindowFlags_NoMove |
          ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoSavedSettings |
          ImGuiWindowFlags_NoDocking |
          ImGuiWindowFlags_NoBringToFrontOnFocus
        : ImGuiWindowFlags_NoCollapse;
    ImGui::Begin(m_mainViewFullscreen ? "Main View Fullscreen" : "Main View", nullptr, mainViewFlags);

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("SceneRenderArea", windowSize, false,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    // Benchmark mode renders at a fixed resolution so results do not depend on the window layout.
    const bool benchmarkMode = debug::DebugView::HasInstance() && debug::DebugView::Get().IsBenchmarkMode();
    int width = static_cast<int>(windowSize.x);
    int height = static_cast<int>(windowSize.y);
    if (benchmarkMode)
    {
        width = debug::DebugView::Get().GetBenchmarkRenderWidth();
        height = debug::DebugView::Get().GetBenchmarkRenderHeight();
    }

    // Start of the gpu_frame_ms span (scene and ImGui rendering of the main context).
    PWG_GPU_FRAME_START();

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
        static_assert(DirectionalShadowMap::CascadeCount <= static_cast<int>(debug::ShadowCascadeCount), "Debug View counts at most ShadowCascadeCount cascades");
        PWG_RENDER_PASS(debug::GpuPass::SHADOW, "Render::Shadow");
        for (int cascade = 0; cascade < DirectionalShadowMap::CascadeCount; ++cascade)
        {
            PWG_SHADOW_CASCADE(cascade);
            m_shadowMap->BeginCascade(cascade, environmentLighting, activeCamera);
            m_terrain->DrawShadowMap(m_terrainShadowShader, m_shadowMap->GetLightSpaceMatrix(cascade));
            if (m_vegetationSystem)
            {
                m_vegetationSystem->DrawShadow(m_shadowMap->GetLightSpaceMatrix(cascade));
            }
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
    {
        PWG_RENDER_PASS(debug::GpuPass::TERRAIN, "Render::Terrain");
        m_renderer.DrawAll();
    }
    if (m_vegetationSystem)
    {
        PWG_RENDER_PASS(debug::GpuPass::VEGETATION, "Render::Vegetation");
        m_vegetationSystem->Draw(m_renderer, m_terrain);
    }
    {
        PWG_RENDER_PASS(debug::GpuPass::WATER, "Render::Water");
        m_terrain->DrawWater(m_renderer);
    }
    m_renderer.EndFrame();
    m_hdrFrameBuffer->Unbind();

    m_frameBuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    {
        PWG_RENDER_PASS(debug::GpuPass::POST_PROCESS, "Render::Post");
        m_postProcessor->Render(m_hdrFrameBuffer->GetTextureID(), m_environment->GetSettings(), width, height);
    }
    m_frameBuffer->Unbind();

    // D-07: a requested screenshot is read here, after post-processing and before ImGui renders, so it holds
    // the scene only. Requests are refused during CSV recordings, so no measured frame contains the readback.
    if (debug::DebugView::HasInstance() && debug::DebugView::Get().GetExporters().HasPendingScreenshot())
    {
        debug::DebugView::Get().CaptureScreenshot(
            MakeScreenshotSource(*m_cameraManager, *m_terrain, m_frameBuffer->GetTextureID(), width, height));
    }

    ImVec2 imageSize = windowSize;
    if (benchmarkMode && width > 0 && height > 0 && windowSize.x > 0.0f && windowSize.y > 0.0f)
    {
        // Fit the fixed-size image into the view, keeping its aspect ratio.
        const float scale = std::min(windowSize.x / static_cast<float>(width), windowSize.y / static_cast<float>(height));
        imageSize = ImVec2(static_cast<float>(width) * scale, static_cast<float>(height) * scale);
        ImGui::SetCursorPos(ImVec2((windowSize.x - imageSize.x) * 0.5f, (windowSize.y - imageSize.y) * 0.5f));
    }
    ImGui::Image(
        (ImTextureID)(intptr_t)m_frameBuffer->GetTextureID(),
        imageSize,
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::EndChild();

    ImGui::End();
    }

    if (m_mainViewFullscreen)
    {
        ImGui::PopStyleVar();
        return;
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
        if (debug::DebugView::HasInstance() && ImGui::BeginTabItem("Performance"))
        {
            debug::DebugView::Get().DrawPerformanceTab();
            ImGui::EndTabItem();
        }
        if (debug::DebugView::HasInstance() && ImGui::BeginTabItem("Render"))
        {
            debug::DebugView::Get().DrawRenderTab(m_terrain ? m_terrain->GetChunkManager().get() : nullptr);
            ImGui::EndTabItem();
        }
        if (debug::DebugView::HasInstance() && ImGui::BeginTabItem("Simulation"))
        {
            debug::DebugView::Get().DrawSimulationTab();
            ImGui::EndTabItem();
        }
        if (debug::DebugView::HasInstance() && ImGui::BeginTabItem("Capture"))
        {
            debug::DebugView::Get().DrawCaptureTab(m_terrain ? m_terrain->GetChunkManager().get() : nullptr);
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
    if (controls::TerrainGraphEditor::Show(m_terrain->GetChunkManager(), m_terrainGraphViewState, m_vegetationAssetRegistry, &terrainGraphInputCaptured, &terrainGraphViewChanged))
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

	if (m_showVegetationLibrary)
	{
		DrawVegetationAssetLibrary();
	}
	if (m_showCameraSettings)
	{
		DrawCameraSettings();
	}

    DrawWorldWindow();
}

void pwg::EditorScene::NewProject()
{
    m_terrain->GetChunkManager()->ConfigureTerrain(31, 129);
    if (auto terrainPipeline = m_terrain->GetChunkManager()->GetTerrainComputePipeline())
    {
        terrainPipeline->ResetUniversalGraphToDefault();
    }
	controls::TerrainGraphEditor::ResetHistory();
    m_terrainGraphViewState = controls::TerrainGraphViewState{};

	m_environment->ResetToDefaults();
	if (m_terrainMaterialLibrary)
	{
		m_terrainMaterialLibrary->ResetToDefaults();
		m_terrainMaterialLibrary->RefreshCatalog();
	}
    if (m_vegetationSystem)
    {
        m_vegetationSystem->Clear();
    }
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
	auto skybox = textureManager->GetCubeMapTexture("Skybox");
    m_skybox->SetCubeMapTexture(skybox);
	if (auto waterMaterial = m_resourceManager->GetMaterialManager()->GetMaterial("WaterMaterial"))
	{
		waterMaterial->SetCubeMapTexture("u_skybox", skybox);
	}
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

void pwg::EditorScene::ReloadVegetationAssets()
{
	if (m_vegetationAssetRegistry)
	{
		m_vegetationAssetRegistry->Reload();
	}
	if (m_vegetationSystem)
	{
		m_vegetationSystem->ReloadAssetModels();
	}
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
bool& pwg::EditorScene::ShowDebugWindow()
{
    return m_showDebugWindow;
}

bool& pwg::EditorScene::ShowMaterialLibrary()
{
    return m_showMaterialLibrary;
}

bool& pwg::EditorScene::ShowCameraSettings()
{
	return m_showCameraSettings;
}

bool& pwg::EditorScene::ShowVegetationLibrary()
{
	return m_showVegetationLibrary;
}

bool& pwg::EditorScene::ShowTerrainGraph()
{
    return m_showTerrainGraph;
}

void pwg::EditorScene::SetMainViewFullscreen(bool fullscreen)
{
    m_mainViewFullscreen = fullscreen;
}

bool pwg::EditorScene::IsMainViewFullscreen() const
{
    return m_mainViewFullscreen;
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
    m_showDebugWindow = true;
	m_showMaterialLibrary = true;
	m_showVegetationLibrary = true;
	m_showCameraSettings = true;
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
        ImGui::TreePop();
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

void pwg::EditorScene::DrawCameraSettings()
{
	ImGui::Begin("Camera Settings", &m_showCameraSettings);
	bool changed = false;
	auto commonSettings = [&](CameraProperties& properties)
	{
		changed |= ImGui::SliderFloat("Field of view", &properties.fov, 35.0f, 100.0f, "%.0f deg");
		changed |= ImGui::DragFloat("Near clip", &properties.nearPlane, 0.01f, 0.01f, 5.0f, "%.2f");
		changed |= ImGui::DragFloat("Far clip", &properties.farPlane, 100.0f, 1000.0f, 50000.0f, "%.0f");
		properties.farPlane = std::max(properties.farPlane, properties.nearPlane + 1.0f);
	};

	if (ImGui::BeginTabBar("CameraSettingsTabs"))
	{
		if (ImGui::BeginTabItem("Editor"))
		{
			auto camera = m_cameraManager->GetEditorCamera();
			auto& settings = camera->GetProperties();
			commonSettings(camera->GetCameraProperties());
			ImGui::Separator();
			changed |= ImGui::SliderFloat("Orbit sensitivity", &settings.orbitSensitivity, 0.02f, 0.50f, "%.2f");
			changed |= ImGui::SliderFloat("Pan speed", &settings.panSpeed, 0.0001f, 0.0050f, "%.4f");
			changed |= ImGui::SliderFloat("Zoom speed", &settings.zoomSpeed, 5.0f, 200.0f, "%.0f");
			changed |= ImGui::DragFloat("Minimum zoom", &settings.minZoom, 1.0f, 1.0f, 1000.0f, "%.0f");
			changed |= ImGui::DragFloat("Maximum zoom", &settings.maxZoom, 10.0f, 100.0f, 50000.0f, "%.0f");
			settings.maxZoom = std::max(settings.maxZoom, settings.minZoom + 1.0f);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Flyover"))
		{
			auto camera = m_cameraManager->GetFlyoverCamera();
			auto& settings = camera->GetProperties();
			commonSettings(camera->GetCameraProperties());
			ImGui::Separator();
			changed |= ImGui::SliderFloat("Mouse sensitivity", &settings.sensitivity, 0.02f, 0.50f, "%.2f");
			changed |= ImGui::SliderFloat("Acceleration", &settings.acceleration, 10.0f, 800.0f, "%.0f");
			changed |= ImGui::SliderFloat("Maximum speed", &settings.maxSpeed, 10.0f, 1000.0f, "%.0f");
			changed |= ImGui::SliderFloat("Flight drag", &settings.flightDrag, 0.0f, 5.0f, "%.2f");
			changed |= ImGui::SliderFloat("Braking", &settings.braking, 0.2f, 12.0f, "%.2f");
			changed |= ImGui::SliderFloat("Boost multiplier", &settings.boostMultiplier, 1.0f, 8.0f, "%.1fx");
			ImGui::Separator();
			changed |= ImGui::SliderFloat("Look smoothing", &settings.lookSmoothing, 1.0f, 30.0f, "%.1f");
			changed |= ImGui::SliderFloat("Gimbal bank", &settings.gimbalStrengthDegrees, 0.0f, 12.0f, "%.1f deg");
			changed |= ImGui::SliderFloat("Gimbal smoothing", &settings.gimbalSmoothing, 1.0f, 20.0f, "%.1f");
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Player"))
		{
			auto camera = m_cameraManager->GetPlayerCamera();
			auto& settings = camera->GetProperties();
			commonSettings(camera->GetCameraProperties());
			ImGui::Separator();
			changed |= ImGui::SliderFloat("Mouse sensitivity", &settings.cameraSensitivity, 0.02f, 0.50f, "%.2f");
			changed |= ImGui::SliderFloat("Acceleration", &settings.acceleration, 10.0f, 200.0f, "%.0f");
			changed |= ImGui::SliderFloat("Maximum speed", &settings.maxGroundSpeed, 5.0f, 100.0f, "%.0f");
			changed |= ImGui::SliderFloat("Ground damping", &settings.groundDamping, 1.0f, 30.0f, "%.1f");
			changed |= ImGui::SliderFloat("Air damping", &settings.airDamping, 0.0f, 8.0f, "%.1f");
			changed |= ImGui::SliderFloat("Gravity", &settings.gravity, 10.0f, 200.0f, "%.0f");
			changed |= ImGui::SliderFloat("Jump speed", &settings.jumpSpeed, 5.0f, 80.0f, "%.0f");
			changed |= ImGui::SliderFloat("Eye height", &settings.eyeHeight, 1.0f, 15.0f, "%.1f");
			changed |= ImGui::SliderFloat("Ground snap", &settings.groundSnapDistance, 0.1f, 6.0f, "%.1f");
			ImGui::Separator();
			changed |= ImGui::Checkbox("Head motion", &settings.headBobEnabled);
			ImGui::BeginDisabled(!settings.headBobEnabled);
			changed |= ImGui::SliderFloat("Step height", &settings.headBobHeight, 0.0f, 0.5f, "%.2f");
			changed |= ImGui::SliderFloat("Step sway", &settings.headBobSide, 0.0f, 0.3f, "%.2f");
			changed |= ImGui::SliderFloat("Stride length", &settings.headBobStrideLength, 2.0f, 12.0f, "%.1f");
			changed |= ImGui::SliderFloat("Step roll", &settings.headBobRollDegrees, 0.0f, 2.0f, "%.2f deg");
			changed |= ImGui::SliderFloat("Motion smoothing", &settings.headBobResponse, 2.0f, 25.0f, "%.1f");
			ImGui::EndDisabled();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	if (changed) m_dirty = true;
	ImGui::End();
}

void pwg::EditorScene::DrawVegetationAssetLibrary()
{
	ImGui::Begin("Vegetation Library", &m_showVegetationLibrary);
	if (!m_vegetationAssetRegistry)
	{
		ImGui::TextDisabled("Asset registry unavailable");
		ImGui::End();
		return;
	}

	static std::string pendingModelPath;
	static char importName[128]{};
	static int importKind = 0;
	static float importHeight = 30.0f;
	static float importGroundSink = 1.0f;
	static glm::vec3 importRotation{ 0.0f };
	static bool importVariants = false;
	static std::string selectedAssetId;
	static char editName[128]{};
	static int editKind = 0;
	static float editHeight = 1.0f;
	static float editGroundSink = 0.0f;
	static glm::vec3 editRotation{ 0.0f };
	static bool editVariants = false;
	auto kindIndex = [](VegetationKind kind)
	{
		if (kind == VegetationKind::Grass) return 1;
		if (kind == VegetationKind::Rock) return 2;
		return 0;
	};
	auto kindFromIndex = [](int index)
	{
		if (index == 1) return VegetationKind::Grass;
		if (index == 2) return VegetationKind::Rock;
		return VegetationKind::Tree;
	};
	auto kindName = [](VegetationKind kind)
	{
		if (kind == VegetationKind::Grass) return "Grass";
		if (kind == VegetationKind::Rock) return "Rock";
		return "Tree";
	};
	auto loadEditState = [&](const VegetationAssetDefinition* asset)
	{
		if (!asset) return;
		std::snprintf(editName, sizeof(editName), "%s", asset->name.c_str());
		editKind = kindIndex(asset->kind);
		editHeight = asset->targetHeight;
		editGroundSink = asset->groundSink;
		editRotation = asset->rotationDegrees;
		editVariants = asset->submeshesAsVariants;
	};

	if (ImGui::Button("Import Model..."))
	{
		pendingModelPath = platform::OpenVegetationModelDialog();
		if (!pendingModelPath.empty())
		{
			std::string defaultName = std::filesystem::path(pendingModelPath).stem().string();
			std::snprintf(importName, sizeof(importName), "%s", defaultName.c_str());
			importKind = 0;
			importHeight = 30.0f;
			importGroundSink = 1.0f;
			importRotation = glm::vec3(0.0f);
			importVariants = false;
			ImGui::OpenPopup("Import Vegetation Model");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload"))
	{
		m_vegetationAssetRegistry->Reload();
		loadEditState(m_vegetationAssetRegistry->Find(selectedAssetId));
	}
	ImGui::TextWrapped("%s", m_vegetationAssetRegistry->Status().c_str());
	ImGui::TextDisabled("%s", m_vegetationAssetRegistry->LibraryRoot().string().c_str());
	if (m_vegetationSystem)
	{
		ImGui::TextDisabled("Runtime: %s", m_vegetationSystem->GetStatus().c_str());
	}

	ImGui::Separator();
	if (ImGui::BeginTable("VegetationAssets", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("Height", ImGuiTableColumnFlags_WidthFixed, 65.0f);
		ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableHeadersRow();
		for (const auto& asset : m_vegetationAssetRegistry->Assets())
		{
			ImGui::PushID(asset.id.c_str());
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			bool selected = asset.id == selectedAssetId;
			if (ImGui::Selectable(asset.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAssetId = asset.id;
				loadEditState(&asset);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", asset.id.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(kindName(asset.kind));
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.1f", asset.targetHeight);
			ImGui::TableSetColumnIndex(3);
			ImGui::TextUnformatted(asset.builtin ? "Built-in" : "External");
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (const VegetationAssetDefinition* selected = m_vegetationAssetRegistry->Find(selectedAssetId))
	{
		ImGui::Separator();
		ImGui::Text("Asset: %s", selected->id.c_str());
		if (selected->builtin)
		{
			ImGui::TextDisabled("Built-in asset metadata is read-only");
		}
		else
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputText("Name", editName, sizeof(editName));
			const char* editKinds[] = { "Tree", "Grass", "Rock" };
			ImGui::Combo("Kind", &editKind, editKinds, IM_ARRAYSIZE(editKinds));
			ImGui::DragFloat("Target Height", &editHeight, 0.1f, 0.1f, 500.0f, "%.2f");
			ImGui::DragFloat("Ground Sink", &editGroundSink, 0.01f, 0.0f, 20.0f, "%.2f");
			ImGui::DragFloat3("Rotation X/Y/Z", &editRotation.x, 1.0f, -180.0f, 180.0f, "%.0f deg");
			if (ImGui::Button("Z-up to Y-up")) editRotation.x = -90.0f;
			ImGui::SameLine();
			if (ImGui::Button("Reset rotation")) editRotation = glm::vec3(0.0f);
			if (editKind != 0)
			{
				ImGui::Checkbox("Submeshes As Variants", &editVariants);
			}
			else
			{
				editVariants = false;
			}
			if (ImGui::Button("Save Manifest"))
			{
				if (m_vegetationAssetRegistry->Update(
					selectedAssetId,
					editName,
					kindFromIndex(editKind),
					editHeight,
					editGroundSink,
					editRotation,
					editVariants))
				{
					if (m_vegetationSystem) m_vegetationSystem->ReloadAssetModels();
					loadEditState(m_vegetationAssetRegistry->Find(selectedAssetId));
				}
			}
		}
	}

	if (ImGui::BeginPopupModal("Import Vegetation Model", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(std::filesystem::path(pendingModelPath).filename().string().c_str());
		ImGui::TextWrapped("%s", m_vegetationAssetRegistry->Status().c_str());
		ImGui::SetNextItemWidth(320.0f);
		ImGui::InputText("Name", importName, sizeof(importName));
		const char* kinds[] = { "Tree", "Grass", "Rock" };
		ImGui::Combo("Kind", &importKind, kinds, IM_ARRAYSIZE(kinds));
		if (ImGui::IsItemEdited())
		{
			if (importKind == 0)
			{
				importHeight = 30.0f;
				importGroundSink = 1.0f;
			}
			else if (importKind == 1)
			{
				importHeight = 3.2f;
				importGroundSink = 0.12f;
			}
			else
			{
				importHeight = 3.0f;
				importGroundSink = 0.15f;
			}
		}
		ImGui::DragFloat("Target Height", &importHeight, 0.1f, 0.1f, 500.0f, "%.2f");
		ImGui::DragFloat("Ground Sink", &importGroundSink, 0.01f, 0.0f, 20.0f, "%.2f");
		ImGui::DragFloat3("Rotation X/Y/Z", &importRotation.x, 1.0f, -180.0f, 180.0f, "%.0f deg");
		if (ImGui::Button("Z-up to Y-up")) importRotation.x = -90.0f;
		ImGui::SameLine();
		if (ImGui::Button("Reset rotation")) importRotation = glm::vec3(0.0f);
		if (importKind != 0)
		{
			ImGui::Checkbox("Submeshes As Variants", &importVariants);
		}
		else
		{
			importVariants = false;
		}
		if (ImGui::Button("Import"))
		{
			auto imported = m_vegetationAssetRegistry->Import(
				pendingModelPath,
				importName,
				kindFromIndex(importKind),
				importHeight,
				importGroundSink,
				importRotation,
				importVariants);
			if (imported)
			{
				selectedAssetId = *imported;
				loadEditState(m_vegetationAssetRegistry->Find(selectedAssetId));
				pendingModelPath.clear();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			pendingModelPath.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
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









