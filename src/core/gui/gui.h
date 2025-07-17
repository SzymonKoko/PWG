#ifndef SRC_CORE_GUI_GUI_H
#define SRC_CORE_GUI_GUI_H

#include "graphics/scene/scene.h"
#include "core/window/window.h"
#include <memory>

namespace pwg
{
	class Gui
	{
	public:

		/**
		* @brief Gui constructor
		* @param window Window that is used for application
		* @param scene Shared pointer to the scene
		*/
		Gui(Window& window, std::shared_ptr<Scene> scene);

		/**
		* @brief Gui destructor
		*/
		~Gui();

		/**
		* @brief Upadtes Gui on every frame
		*/
		void Update();

		/**
		* @brief Renders Gui to the screen
		*/
		void Render();
	private:
		pwg::Window& m_window;
		std::shared_ptr<Scene> m_scene;

		/**
		* @brief Enables and sets up dock space 
		*/
		void EnableDockSpace();
	};
}

#endif // !SRC_CORE_GUI_GUI_H