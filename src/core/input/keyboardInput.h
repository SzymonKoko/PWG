#ifndef SRC_CORE_INPUT_KEYBOARD_INPUT_H_
#define SRC_CORE_INPUT_KEYBOARD_INPUT_H_

#include <GLFW/glfw3.h>
#include <core/logger/logger.h>
#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>

namespace pwg
{
	/**
	* @brief Enum that stores available in-game actions which can be bound to keyboard keys.
	*/
	enum class Action
	{
		MoveForward = 0,	/**< Moves the player or camera forward */
		MoveBackward,		/**< Moves the player or camera backward */
		MoveLeft,			/**< Moves the player or camera to the left */
		MoveRight,			/**< Moves the player or camera to the right */
		MoveUp,				/**< Moves upward */
		MoveDown,			/**< Moves downward */
		Exit,				/**< Exits the application */
		EditorCamera,
		PlayerCamera,
		ToggleDebug,		/**< Toggles debug information or mode */
		Count				/**< Helper value representing number of available actions */
	};

	/**
	* @brief Precompiled number of available actions in the Action enum.
	*/
	constexpr int NumActions = static_cast<int>(pwg::Action::Count);

	/**
	* @brief Handles keyboard input logic. Manages key bindings, press states, and actions.
	*/
	class KeyboardInput
	{
	public:

		/**
		* @brief Constructs the keyboard input handler.
		* @param window Pointer to the GLFW window used for input context.
		*/
		KeyboardInput(GLFWwindow* window);

		/**
		* @brief Default destructor for the keyboard input handler.
		*/
		~KeyboardInput() = default;

		/**
		* @brief Updates the keyboard input state each frame.
		* Copies current key states and stores previous ones for press/release detection.
		*/
		void Update();

		/**
		* @brief Checks if a specific action was pressed in the current frame.
		* @param action The action to check.
		* @return True if the action key was pressed, false otherwise.
		*/
		bool IsPressed(Action action) const;

		/**
		* @brief Checks if a specific action key is being held down.
		* @param action The action to check.
		* @return True if the action key is held down, false otherwise.
		*/
		bool IsHeld(Action action) const;

		/**
		* @brief Checks if a specific action key was released in the current frame.
		* @param action The action to check.
		* @return True if the action key was released, false otherwise.
		*/
		bool IsReleased(Action action) const;

		/**
		* @brief Changes the key binding for a given action.
		* @param action The action to rebind.
		* @param newKey The new GLFW key code to assign.
		*/
		void RebindKey(Action action, int newKey);

		/**
		* @brief Retrieves the GLFW key code currently bound to an action.
		* @param action The action to check.
		* @return Integer representing the bound key code.
		*/
		int GetKeyForAction(Action action) const;

		/**
		* @brief Updates the GLFW window reference used for input detection.
		* @param window Pointer to the new GLFW window instance.
		*/
		void SetWindow(GLFWwindow* window);

	private:
		GLFWwindow* m_window = nullptr;		/**< Pointer to the GLFW window context */
		bool m_currentState[NumActions]{};	/**< Current frame key states for each action */
		bool m_previousState[NumActions]{};	/**< Previous frame key states for each action */
		int m_keyBindings[NumActions]{};	/**< Array storing GLFW key bindings for each action */
	};
}

#endif // !SRC_CORE_INPUT_KEYBOARD_INPUT_H_
