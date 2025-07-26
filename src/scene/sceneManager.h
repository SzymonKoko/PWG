#ifndef SCENE_SCENE_MANAGER_H
#define SCENE_SCENE_MANAGER_H

#include "scene/iscene.h"
#include "scene/editorScene.h"
#include "scene/playScene.h"

namespace pwg 
{
	class SceneManager
	{
	public:

		/**
		* @brief SceneManager constructor
		* @param window Pointer to the GLFWwindow that is used for camera
		* @param minput Pointer to the mouse input that is used for camera
		* @param kinput Pointer to the keyboard input that is used for camera
		*/
		SceneManager(GLFWwindow* window, MouseInput* minput, KeyboardInput* kinput);

		/**
		* @brief Default SceneManager destructor. Does nothing.
		*/
		~SceneManager() = default;

		/**
		* @brief Changes mode from editor to runtime
		*/
		void Play();

		/**
		* @brief Changes mode from runtime to editor
		*/
		void Stop();

		/**
		* @brief Updates scene on every frame
		*/
		void Update(const float& dt);

		/**
		* @brief Draws entities to the scene
		*/
		void Draw();
	private:
		std::unique_ptr<IScene> m_currentScene;
		std::unique_ptr<EditorScene> m_editorScene;

		GLFWwindow* m_window;
		MouseInput* m_mouseInput;
		KeyboardInput* m_keyboardInput;
	};
}
#endif // !SCENE_SCENE_MANAGER_H

