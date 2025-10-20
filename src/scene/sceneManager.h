#ifndef SCENE_SCENE_MANAGER_H
#define SCENE_SCENE_MANAGER_H

#include "scene/iscene.h"
#include "scene/editorScene.h"
#include "scene/playScene.h"

namespace pwg
{
	/**
	 * @brief Manages current scene (editor or runtime) and handles switching between them.
	 */
	class SceneManager
	{
	public:

		/**
		 * @brief Constructs the SceneManager.
		 * @param window Pointer to the GLFWwindow that is used for camera.
		 * @param minput Reference to the mouse input handler.
		 * @param kinput Reference to the keyboard input handler.
		 * @param resourceManager Shared pointer to the resource manager.
		 * @param renderer Reference to the renderer.
		 */
		SceneManager(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput, std::shared_ptr<ResourceManager> resourceManager, Renderer& renderer);

		/**
		 * @brief Default destructor. Cleans up scene references.
		 */
		~SceneManager() = default;

		/**
		 * @brief Switches the scene to runtime/play mode.
		 */
		void Play();

		/**
		 * @brief Switches the scene back to editor mode.
		 */
		void Stop();

		/**
		 * @brief Updates the currently active scene.
		 * @param dt Delta time since last frame.
		 */
		void Update(const float& dt);

		/**
		 * @brief Draws the currently active scene.
		 */
		void Draw();

	private:
		std::unique_ptr<IScene> m_currentScene;				/**< Pointer to the current active scene (editor or play). */
		std::unique_ptr<EditorScene> m_editorScene;			/**< Editor scene instance. */

		GLFWwindow* m_window;								/**< Pointer to GLFW window. */
		MouseInput& m_mouseInput;							/**< Reference to mouse input handler. */
		KeyboardInput& m_keyboardInput;						/**< Reference to keyboard input handler. */
		Renderer& m_renderer;								/**< Reference to renderer. */
		std::shared_ptr<ResourceManager> m_resourceManager; /**< Shared pointer to the resource manager. */
	};
}
#endif // !SCENE_SCENE_MANAGER_H
