#ifndef SRC_SCENE_EDITOR_SCENE_H_
#define SRC_SCENE_EDITOR_SCENE_H_

#include "graphics/framebuffer/frameBuffer.h"
#include "graphics/renderer/renderer.h"
#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "scene/iscene.h"
#include "resources/resourceManager.h"
#include "terrain/terrain.h"
#include "scene/environment/environmentSystem.h"
#include "graphics/postprocess/postProcessor.h"
#include "graphics/shadow/directionalShadowMap.h"
#include "cameras/cameraManager.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/materials/terrainMaterialLibrary.h"
#include "scene/controls/terrainGraphEditor.h"
#include "vegetation/vegetationSystem.h"

#include <array>
#include <memory>
#include <string>

namespace pwg
{
	struct SkyboxDefinition
	{
		std::string name{ "miramar" };
		std::string folder{ "../assets/textures/skybox" };
		std::array<std::string, 6> faces{};
		std::array<int, 6> rotations{};
		std::array<bool, 6> flipHorizontal{};
		std::array<bool, 6> flipVertical{};
	};

	/**
	 * @brief Represents the editor scene.
	 * Manages terrain, entities, cameras, lighting, meshes, input, and rendering.
	 */
	class EditorScene : public IScene
	{
	public:

		/**
		 * @brief Constructs an EditorScene.
		 * @param window Pointer to the GLFW window.
		 * @param minput Reference to the mouse input handler.
		 * @param kinput Reference to the keyboard input handler.
		 * @param resourceManager Shared pointer to the ResourceManager for meshes, textures, shaders, etc.
		 * @param renderer Reference to the Renderer used for drawing objects.
		 */
		EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput,
			std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer);

		/**
		 * @brief Destructor.
		 * Cleans up unique_ptr-managed resources automatically.
		 */
		~EditorScene() = default;

		/**
		 * @brief Updates the scene every frame.
		 * @param dt Delta time since last frame.
		 */
		void Update(const float& dt) override;

		/**
		 * @brief Draws all scene objects.
		 */
		void Draw() override;

		void NewProject();
		std::shared_ptr<Terrain> GetTerrain();
		std::shared_ptr<EnvironmentSystem> GetEnvironmentSystem();
		CameraManager* GetCameraManager();
		std::shared_ptr<TerrainMaterialLibrary> GetTerrainMaterialLibrary();
		void ReloadVegetationAssets();
		const SkyboxDefinition& GetSkyboxDefinition() const;
		void SetSkyboxDefinition(const SkyboxDefinition& definition);
		bool ReloadSkybox();
		bool ImportSkyboxImage(const std::string& imagePath);
		const std::string& GetProjectLayoutIni() const;
		void SetProjectLayoutIni(const std::string& layoutIni);
		controls::TerrainGraphViewState& GetTerrainGraphViewState();
		const controls::TerrainGraphViewState& GetTerrainGraphViewState() const;
		void SetTerrainGraphViewState(const controls::TerrainGraphViewState& viewState);
		void ResetEditorWindows();
		bool ConsumeDirtyFlag();
		bool& ShowMainView();
		bool& ShowTerrainControls();
		bool& ShowDebugWindow();
		bool& ShowMaterialLibrary();
		bool& ShowCameraSettings();
		bool& ShowVegetationLibrary();
		bool& ShowTerrainGraph();
		bool& ShowWorld();
		bool& ShowOutliner();
		void SetMainViewFullscreen(bool fullscreen);
		bool IsMainViewFullscreen() const;

	private:

		/**
		 * @brief Handles keyboard input for the scene.
		 */
		void HandleKeyboardInputs();
		void DrawOutliner();
		void DrawWorldWindow();
		void DrawCameraSettings();
		void DrawSkyboxControls();
		void DrawVegetationAssetLibrary();
		bool LoadSkyboxFolder(const std::string& folder);
		void ResetSkyboxToDefault();

		GLFWwindow* m_window;									/**< Pointer to the GLFW window. */
		MouseInput& m_mouseInput;								/**< Reference to the mouse input handler. */
		KeyboardInput& m_keyboardInput;							/**< Reference to the keyboard input handler. */
		Renderer& m_renderer;									/**< Reference to the renderer. */
		std::shared_ptr<ResourceManager> m_resourceManager;		/**< Shared pointer to the resource manager. */

		std::unique_ptr<FrameBuffer> m_frameBuffer;				/**< Framebuffer used for editor rendering. */
		std::unique_ptr<FrameBuffer> m_hdrFrameBuffer;			/**< HDR scene framebuffer before postprocessing. */
		std::shared_ptr<Terrain> m_terrain;						/**< Terrain instance in the scene. */
		//std::shared_ptr<TerrainComputePipeline> m_terrainComputePipeline;
		std::unique_ptr<MeshManager> m_meshManager;				/**< Manages procedural and loaded meshes. */
		std::unique_ptr<CameraManager> m_cameraManager;			/**< Handles active camera and switching between cameras. */
		std::shared_ptr<EnvironmentSystem> m_environment;		/**< World lighting, sky and postprocess settings. */
		std::shared_ptr<PostProcessor> m_postProcessor;			/**< HDR to viewport postprocess pass. */
		std::shared_ptr<DirectionalShadowMap> m_shadowMap;		/**< Directional sunlight shadow map. */
		std::shared_ptr<Shader> m_terrainShadowShader;			/**< Depth-only terrain shader for shadow pass. */
		std::shared_ptr<SkyBox> m_skybox;
		std::shared_ptr<TerrainMaterialLibrary> m_terrainMaterialLibrary;
		std::shared_ptr<VegetationAssetRegistry> m_vegetationAssetRegistry;
		std::shared_ptr<VegetationSystem> m_vegetationSystem;
		SkyboxDefinition m_skyboxDefinition;
		std::string m_skyboxStatus{ "Ready" };
		std::string m_projectLayoutIni;
		controls::TerrainGraphViewState m_terrainGraphViewState;


		float m_aspectRatio = 16.0f / 9.0f;						/**< Aspect ratio of the viewport. */
		bool m_showMainView{ true };
		bool m_showOutliner{ true };
		bool m_showTerrainControls{ false };
		bool m_showWorld{ true };
		bool m_showDebugWindow{ true };
		bool m_showMaterialLibrary{ true };
		bool m_showCameraSettings{ true };
		bool m_showVegetationLibrary{ true };
		bool m_showTerrainGraph{ true };
		bool m_mainViewFullscreen{ false };
		bool m_terrainGraphInputCaptured{ false };
		bool m_dirty{ false };
		bool m_viewportInputCaptured{ false };
	};
} // namespace pwg

#endif // !SRC_SCENE_EDITOR_SCENE_H_


