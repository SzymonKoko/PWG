#ifndef SRC_CORE_EDITOR_IEDITOR_H
#define SRC_CORE_EDITOR_IEDITOR_H

#include "core/window/window.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"



namespace pwg
{
	/**
	* @brief Interface to handle different editors of the game (WindowEditor, TerrainEditor etc.)
	*        Each concrete class shall provide functions to init, update and render.
	*/
	class IEditor
	{
	public:
		/**
		 * @brief Default destructor. Does nothing.
		 */
		~IEditor() = default;

		/**
		 * @brief Initializing editor with data.
		 */
		virtual void InitEditor() = 0;

		/**
		 * @brief Updates the internal editor.
		 * @param dt Time of the cycle (elapsed time since last cycle) in seconds.
		 */
		virtual void Update(const float dt) = 0;

		/**
		 * @brief Renders the internal editor.
		 */
		virtual void Render() = 0;

		virtual const char* GetName() const = 0;

		/**
		 * @brief Indicates if the editor is active.
		 */
		virtual bool IsActive() const { return m_isActive; }

		/**
		 * @brief Sets the active state of the editor.
		 */
		virtual void SetActive(bool active) { m_isActive = active; }
	protected:
		bool m_isActive{ false };

	};
}
#endif // !SRC_CORE_EDITOR_IEDITOR_H
