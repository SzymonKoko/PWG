#ifndef SRC_CORE_GUI_GUI_H
#define SRC_CORE_GUI_GUI_H

#include "scene/sceneManager.h"
#include "core/window/window.h"
#include "graphics/texture/texture.h"
#include <memory>
#include <string>
#include <vector>

namespace pwg::debug
{
	class Metadata;
}

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

		// ---- Experiment runner access (D-11). The runner reuses the editor's own project and window paths. ----

		/**
		* @brief Opens a project file exactly like File > Open Recent (editor state, layout, clean flag). The runner passes
		* `addToRecent` false, so its scratch copies do not end up in the user's recent project list.
		*/
		bool OpenProjectFromPath(const std::string& path, bool addToRecent = true);

		/**
		* @brief Writes the project and its simulation sidecar to `path`, like Save As, but keeps the current project
		* path and the unsaved-changes flag: the runner's "save" action never touches the project that was loaded.
		* The window layout in the copy is the one of the loaded project, not the current ImGui layout.
		*/
		bool SaveProjectCopy(const std::string& path);

		/** @brief The same switch as F10: fullscreen window and fullscreen main view. No effect without an editor scene. */
		void SetMainViewFullscreen(bool fullscreen);

		/** @brief Closes the application without the unsaved-changes question (runner `--exit-when-done`). */
		void ForceClose();

	private:

		pwg::Window& m_window;						/**< Reference to the main application window used by the GUI */
		std::shared_ptr<SceneManager> m_scene;		/**< Shared pointer to the active scene manager instance */
		std::string m_currentProjectPath;
		std::string m_projectStatus;
		std::vector<std::string> m_recentProjects;
		std::shared_ptr<Texture> m_welcomeTexture;
		bool m_showWelcomeScreen{ true };
		bool m_showAbout{ false };
		bool m_isProjectDirty{ false };
		bool m_suppressDirtyDetection{ false };
		bool m_allowWindowClose{ false };
		bool m_dockLayoutInitialized{ false };
		float m_frameDeltaTime{ 0.016f };

		enum class PendingProjectAction
		{
			NONE,
			NEW_PROJECT,
			OPEN_PROJECT,
			OPEN_RECENT,
			EXIT_APP
		};
		PendingProjectAction m_pendingAction{ PendingProjectAction::NONE };
		std::string m_pendingPath;

		/**
		* @brief Enables and sets up ImGui dock space for editor panels.
		*/
		void EnableDockSpace();
		void ApplyDarkGraphiteStyle();
		void DrawMainMenuBar();
		void DrawTopToolbar();
		void DrawStatusBar();
		void DrawWelcomeScreen();
		void DrawAboutWindow();
		void DrawUnsavedChangesPopup();
		void HandleWindowCloseRequest();
		void CaptureProjectLayout();
		void ApplyProjectLayout();
		bool NewProject();
		bool OpenProject();
		bool SaveProject();
		bool SaveProjectAs();
		void RequestProjectAction(PendingProjectAction action, const std::string& path = {});
		void ExecutePendingProjectAction();
		void MarkProjectDirty();
		void MarkProjectClean();
		void AddRecentProject(const std::string& path);
		void LoadRecentProjects();
		void SaveRecentProjects();

		/**
		* @brief Scene part of the CSV metadata header (D-09): project file and its SHA-256, the simulation
		* sidecar's SHA-256, unsaved changes, world size and fullscreen main view.
		*/
		void FillRunMetadata(debug::Metadata& metadata);
		std::string RecentProjectsPath();
		std::string OpenProjectDialog();
		std::string SaveProjectDialog();
	};
}

#endif // !SRC_CORE_GUI_GUI_H
