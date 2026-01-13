#ifndef SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_
#define SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_

#include "editorCamera.h"
#include "playerCamera.h"
#include <memory>

namespace pwg
{
	/**
	* @brief Enum representing available camera types in the scene.
	*/
	enum class CameraType
	{
		EDITOR = 0, /**< Editor camera for free movement and scene inspection. */
		PLAYER      /**< Player camera for gameplay perspective. */
	};

	/**
	* @brief Class responsible for managing multiple camera instances.
	* Handles switching between editor and player cameras and updating the active camera.
	*/
	class CameraManager
	{
	public:
		/**
		* @brief Constructs the CameraManager with references to input systems.
		* @param mouseInput Reference to the MouseInput system.
		* @param keyboardInput Reference to the KeyboardInput system.
		*/
		CameraManager(pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput);

		/**
		* @brief Destructor. Cleans up camera resources.
		*/
		~CameraManager() = default;

		/**
		* @brief Updates the currently active camera every frame.
		* @param dt Delta time since last frame.
		*/
		void Update(float dt);

		/**
		* @brief Switches the active camera to the specified type.
		* @param type Camera type to activate.
		*/
		void SetCamera(CameraType type);

		/**
		* @brief Sets a default camera position based on scene size.
		* @param size Size of the scene or area to position camera at.
		*/
		void SetDefaultCameraPosition(int size);

		/**
		* @brief Sets the active camera's world position.
		* @param x X coordinate.
		* @param y Y coordinate.
		* @param z Z coordinate.
		*/
		void SetCameraPosition(float x, float y, float z);

		/**
		* @brief Returns the current world position of the active camera.
		* @return 3D vector representing camera position.
		*/
		glm::vec3 GetCameraPosition();

		/**
		* @brief Returns a shared pointer to the active camera instance.
		* @return Shared pointer to ICamera.
		*/
		std::shared_ptr<ICamera> GetActiveCamera();

		/**
		* @brief Returns the type of the currently active camera.
		* @return CameraType reference of active camera.
		*/
		CameraType& GetActiveCameraType();

	private:
		MouseInput& m_mouseInput;						/**< Reference to the MouseInput system for camera control. */
		KeyboardInput& m_keyboardInput;					/**< Reference to the KeyboardInput system for camera control. */
		CameraType m_activeCameraType;					/**< Currently active camera type. */

		std::shared_ptr<EditorCamera> m_editorCamera;	/**< Editor camera instance. */
		std::shared_ptr<PlayerCamera> m_playerCamera;	/**< Player camera instance. */
		std::shared_ptr<ICamera> m_activeCamera;		/**< Currently active camera pointer. */
	};
}
#endif // !SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_
