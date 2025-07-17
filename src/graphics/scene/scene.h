#ifndef SRC_GRAPHICS_SCENE_SCENE_H
#define SRC_GRAPHICS_SCENE_SCENE_H

#include "graphics/buffer/frameBuffer.h"
#include "graphics/renderer/renderer.h"
#include "graphics/camera/camera.h"
#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"

#include <memory>

namespace pwg
{
	class Scene
	{
	public:

		/**
		* @brief Scene constructor
		* @param window GLFWwindow that is used for camera
		* @param minput Mouse input that is used for camera
		* @param kinput Keyboard input that is used for camera
		*/
		Scene(GLFWwindow* window, MouseInput* minput, KeyboardInput* kinput);

		/**
		* @brief Default destructor, does nothing.
		*/
		~Scene() = default;

		/**
		* @brief Updates scene on every frame
		*/
		void Update(const float& dt);

		/**
		* @brief Draws entities to the scene
		*/
		void Draw();
	private:
		Camera m_camera;
		Renderer m_renderer;
		std::unique_ptr<FrameBuffer> m_frameBuffer;

		GLFWwindow* m_window;
		MouseInput* m_mouseInput;
		KeyboardInput* m_keyboardInput;
	};
}
#endif // !SRC_GRAPHICS_SCENE_SCENE_H
