#ifndef SRC_SCENE_EDITOR_SCENE_H_
#define SRC_SCENE_EDITOR_SCENE_H_

#include "graphics/buffer/frameBuffer.h"
#include "graphics/renderer/renderer.h"
#include "core/window/window.h"
#include "graphics/camera/camera.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "scene/iscene.h"
#include "editor/editorCamera/editorCamera.h"
#include "procedural/noiseTexture.h"

#include <memory>

namespace pwg
{
	class EditorScene : public IScene
	{
	public:

		/**
		* @brief EditorScene constructor
		* @param window GLFWwindow that is used for camera
		* @param minput Mouse input that is used for camera
		* @param kinput Keyboard input that is used for camera
		*/
		EditorScene(GLFWwindow* window, MouseInput* minput, KeyboardInput* kinput);

		/**
		* @brief EditorScene copy constructor. Creates a deep copy of a given EditorScene object
		* @param otherEditorScene EditorScene instance to copy from
		*/
		EditorScene(const EditorScene& otherEditorScene);

		/**
		* @brief Copy assignment operator for EditorScene
		* @param otherEditorScene EditorScene instance to copy from
		* @return A reference to this EditorScene instance
		*/
		EditorScene& operator=(const EditorScene& otherEditorScene);

		/**
		* @brief Default destructor, does nothing.
		*/
		~EditorScene() = default;

		/**
		* @brief Updates scene on every frame
		*/
		void Update(const float& dt) override;

		/**
		* @brief Draws entities to the scene
		*/
		void Draw() override;

		/**
		* @brief Clones the current editor scene
		* @return A unique_ptr to a deep-copied EditorScene
		*/
		std::unique_ptr<IScene> Clone() override;
		Renderer m_renderer;
		std::unique_ptr<FrameBuffer> m_frameBuffer;
		std::unique_ptr<EditorCamera> m_editorCamera;

		GLFWwindow* m_window;
		MouseInput* m_mouseInput;
		KeyboardInput* m_keyboardInput;

		std::unique_ptr<NoiseTexture> m_noiseTexture;

	};
}

#endif // !SRC_SCENE_EDITOR_SCENE_H_
