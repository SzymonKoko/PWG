#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_

#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/logger/logger.h"
#include "core/gui/gui.h"
#include "scene/sceneManager.h"
#include <entt/entt.hpp>

namespace pwg
{
	class Application
	{
	public:

		/**
		* @brief Constructs the application. Initializes window, input, GUI, and scene manager.
		*/
		Application();

		/**
		* @brief Destructs the application. Cleans up resources and shuts down GLFW.
		*/
		~Application();

		/**
		* @brief Updates the application logic. Processes input, updates scenes, and GUI.
		*/
		void Update();

		/**
		* @brief Renders the current frame. Draws the scene and GUI to the window.
		*/
		void Render();

		/**
		* @brief Runs the main application loop. Calls Update() and Render() every frame until the window is closed.
		*/
		void Run();

	private:
		std::unique_ptr<pwg::Window> m_window;
		std::unique_ptr<pwg::KeyboardInput> m_keyboardInput;
		std::unique_ptr<pwg::MouseInput> m_mouseInput;
		std::unique_ptr<pwg::Gui> m_gui;
		std::shared_ptr<pwg::SceneManager> m_scene;

	};
} //namespace pwg

#endif // !SRC_CORE_APPLICATION_APPLICATION_H_