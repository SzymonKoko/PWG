#ifndef SRC_CORE_GUI_GUI_H
#define SRC_CORE_GUI_GUI_H

#include "scene/sceneManager.h"
#include "core/window/window.h"
#include <memory>

namespace pwg
{
	/**
	* @brief Class responsible for handling and rendering the graphical user interface (GUI) using ImGui.
	*/
	class Gui
	{
	public:

		/**
		* @brief Gui constructor
		* @param window Reference to the main application window
		* @param scene Shared pointer to the scene manager instance
		*/
		Gui(Window& window, std::shared_ptr<SceneManager> scene);

		/**
		* @brief Gui destructor. Cleans up ImGui context and related resources.
		*/
		~Gui();

		/**
		* @brief Updates GUI every frame.
		* @param dt Delta time between frames
		*/
		void Update(const float& dt);

		/**
		* @brief Renders GUI to the screen. Draws all active windows, panels, and dock space.
		*/
		void Render();

	private:

		pwg::Window& m_window;						/**< Reference to the main application window used by the GUI */
		std::shared_ptr<SceneManager> m_scene;		/**< Shared pointer to the active scene manager instance */

		/**
		* @brief Enables and sets up ImGui dock space for editor panels.
		*/
		void EnableDockSpace();
	};
}

#endif // !SRC_CORE_GUI_GUI_H
