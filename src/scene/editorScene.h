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
#include "objects/sunObject.h"
#include "cameras/cameraManager.h"
#include "terrain/gpu/terrainComputePipeline.h"

#include <memory>

namespace pwg
{
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

	private:

		/**
		 * @brief Handles keyboard input for the scene.
		 */
		void HandleKeyboardInputs();

		GLFWwindow* m_window;									/**< Pointer to the GLFW window. */
		MouseInput& m_mouseInput;								/**< Reference to the mouse input handler. */
		KeyboardInput& m_keyboardInput;							/**< Reference to the keyboard input handler. */
		Renderer& m_renderer;									/**< Reference to the renderer. */
		std::shared_ptr<ResourceManager> m_resourceManager;		/**< Shared pointer to the resource manager. */

		std::unique_ptr<FrameBuffer> m_frameBuffer;				/**< Framebuffer used for editor rendering. */
		std::shared_ptr<Terrain> m_terrain;						/**< Terrain instance in the scene. */
		std::shared_ptr<TerrainComputePipeline> m_terrainComputePipeline;
		std::unique_ptr<MeshManager> m_meshManager;				/**< Manages procedural and loaded meshes. */
		std::unique_ptr<CameraManager> m_cameraManager;			/**< Handles active camera and switching between cameras. */
		std::shared_ptr<SunObject> m_sunObject;					/**< Sun object providing lighting. */


		float m_aspectRatio = 16.0f / 9.0f;						/**< Aspect ratio of the viewport. */
	};
} // namespace pwg

#endif // !SRC_SCENE_EDITOR_SCENE_H_
