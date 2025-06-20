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
	//Enum that stores actions which could be binded to a functional key
	enum class Action
	{
		MoveForward = 0,
		MoveBackward,
		MoveLeft,
		MoveRight,
		MoveUp,
		MoveDown,
		Exit,
		Count
	};

	//Precompiled size of a enum class
	constexpr int NumActions = static_cast<int>(pwg::Action::Count);

	class KeyboardInput
	{
	public:

		KeyboardInput(GLFWwindow* window);
		~KeyboardInput() = default;

		void Update();
		bool IsPressed(Action action) const;
		bool IsHeld(Action action) const;
		bool IsReleased(Action action) const;
		void RebindKey(Action action, int newKey);
		int GetKeyForAction(Action action) const;


	private:
		GLFWwindow* m_window = nullptr;
		bool m_currentState[NumActions]{};
		bool m_previousState[NumActions]{};
		int m_keyBindings[NumActions]{};

	};
}

#endif // !SRC_CORE_INPUT_KEYBOARD_INPUT_H_
