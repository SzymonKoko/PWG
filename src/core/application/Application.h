#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_

#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include "core/logger/logger.h"
#include "core/gui/gui.h"
#include "scene/sceneManager.h"
#include <entt/entt.hpp>
#include "resources/resourceManager.h"
#include "graphics/renderer/renderer.h"
#include "debug/debugView.h"
#include "debug/experimentRunner.h"
#include <string>

namespace pwg
{
	/**
	* @brief Command line of the application (D-11, design 8.1): `PWG.exe --experiment <config.yaml> [--out <folder>] [--exit-when-done]`.
	*/
	struct ApplicationOptions
	{
		std::string experimentConfig;		/**< Config of an experiment run, started after the first frame. Empty: normal editor start. */
		std::string outputFolder;			/**< Replaces the config's `output`. */
		bool exitWhenDone{ false };			/**< Close the application when the run is over. */
	};

	/**
	* @brief Main application class responsible for initializing and running core engine systems.
	*/
	class Application
	{
	public:

		/**
		* @brief Constructs the application. Initializes window, input, GUI, and scene manager.
		*/
		explicit Application(ApplicationOptions options = {});

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

		/** @brief Process exit code: 0 normally, 1 if an experiment run was rejected, aborted or had units with problems. */
		int GetExitCode() const;

	private:

		/** @brief Experiment runner step, once per frame after the frame ended (starts the command line run after frame 1). */
		void TickExperiment();

		std::unique_ptr<pwg::Window> m_window;						/**< Pointer to the main application window */
		std::unique_ptr<pwg::debug::DebugView> m_debugView;			/**< Debug View core: frame timing, profilers, benchmark mode */
		std::unique_ptr<pwg::KeyboardInput> m_keyboardInput;		/**< Keyboard input handler */
		std::unique_ptr<pwg::MouseInput> m_mouseInput;				/**< Mouse input handler */
		std::unique_ptr<pwg::Gui> m_gui;							/**< Graphical user interface manager */
		std::shared_ptr<pwg::Renderer> m_renderer;					/**< Rendering subsystem responsible for drawing scenes */
		std::shared_ptr<pwg::SceneManager> m_scene;					/**< Scene manager that handles active scenes and entities */
		std::shared_ptr<pwg::ResourceManager> m_resourceManager;	/**< Resource manager responsible for loading and caching assets */
		std::unique_ptr<pwg::debug::ExperimentRunner> m_runner;		/**< Experiment runner (D-11), driven once per frame */
		ApplicationOptions m_options;
		bool m_experimentStarted{ false };
		bool m_experimentRejected{ false };
		uint64_t m_frames{ 0 };
	};
} // namespace pwg

#endif // !SRC_CORE_APPLICATION_APPLICATION_H_
