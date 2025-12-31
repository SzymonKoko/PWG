#ifndef SRC_SCENE_EDITOR_SCENE_H_
#define SRC_SCENE_EDITOR_SCENE_H_

#include "graphics/framebuffer/frameBuffer.h"
#include "graphics/renderer/renderer.h"
#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include <entt/entt.hpp>
#include "core/ecs/entity.h"
#include "scene/iscene.h"
#include "resources/resourceManager.h"
#include "terrain/terrain.h"
#include "objects/sunObject.h"

#include <memory>

namespace pwg
{
	/**
	 * @brief Represents the editor scene.
	 * Manages entities, terrain, meshes, input, and rendering within the editor.
	 */
	class EditorScene : public IScene
	{
	public:

		/**
		 * @brief Constructs the editor scene.
		 * @param window Pointer to the GLFW window.
		 * @param minput Reference to the mouse input handler.
		 * @param kinput Reference to the keyboard input handler.
		 * @param resourceManager Shared pointer to the resource manager.
		 * @param renderer Reference to the renderer.
		 */
		EditorScene(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer);

		/**
		 * @brief Copy constructor. Performs a deep copy of another EditorScene.
		 * @param otherEditorScene EditorScene instance to copy from.
		 */
		EditorScene(const EditorScene& otherEditorScene);

		/**
		 * @brief Copy assignment operator.
		 * @param otherEditorScene EditorScene instance to copy from.
		 * @return Reference to this EditorScene instance.
		 */
		EditorScene& operator=(const EditorScene& otherEditorScene);

		/**
		 * @brief Default destructor.
		 */
		~EditorScene() = default;

		/**
		 * @brief Updates the editor scene every frame.
		 * @param dt Delta time since last frame.
		 */
		void Update(const float& dt) override;

		/**
		 * @brief Draws all entities and components in the scene.
		 */
		void Draw() override;

		/**
		 * @brief Creates a deep copy of the current scene.
		 * @return Unique pointer to a cloned EditorScene.
		 */
		std::unique_ptr<IScene> Clone() override;

	private:

		GLFWwindow* m_window;									/**< Pointer to the GLFW window. */
		MouseInput& m_mouseInput;								/**< Reference to the mouse input instance. */
		KeyboardInput& m_keyboardInput;							/**< Reference to the keyboard input instance. */
		Renderer& m_renderer;									/**< Reference to the renderer instance. */
		std::shared_ptr<ResourceManager> m_resourceManager;		/**< Shared pointer to the resource manager. */

		std::unique_ptr<FrameBuffer> m_frameBuffer;				/**< Framebuffer used for rendering. */
		std::shared_ptr<Terrain> m_terrain;						/**< Terrain instance of the scene. */
		std::unique_ptr<MeshManager> m_meshManager;				/**< Mesh manager for procedural and loaded meshes. */
		entt::registry m_editorSceneRegistry;					/**< ECS registry storing scene entities. */
		std::shared_ptr<pwg::SunObject> m_sunObject;

		float m_aspectRatio = 16.0f / 9.0f;						/**< Aspect ratio of the viewport. */

		//vector of lights
	};
} // namespace pwg

#endif // !SRC_SCENE_EDITOR_SCENE_H_
