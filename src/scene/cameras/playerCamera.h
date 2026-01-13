#ifndef SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_
#define SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_

#include "icamera.h"
#include "graphics/common/transform.h"

namespace pwg
{
	/**
	* @brief Struct holding properties specific to a player-controlled camera.
	* Includes orientation, movement speed, and mouse sensitivity.
	*/
	struct PlayerCameraProperties
	{
		float yaw{ -90.0f };						/**< Yaw rotation angle in degrees. */
		float pitch{ 90.0f };						/**< Pitch rotation angle in degrees. */
		glm::vec3 direction{ 0.0f, 0.0f, 0.0f };	/**< Forward direction vector for the camera. */

		float cameraSensitivity{ 0.1f };			/**< Mouse sensitivity for looking around. */
		float cameraSpeed{ 2.0f };					/**< Movement speed for WASD/keyboard controls. */
	};

	/**
	* @brief Camera class for a player-controlled FPS-style camera.
	* Inherits from ICamera and implements free movement with keyboard and mouse input.
	*/
	class PlayerCamera : public ICamera
	{
	public:
		PlayerCamera();
		~PlayerCamera() = default;

		/**
		* @brief Updates the camera each frame based on input and delta time.
		* @param dt Delta time since last frame.
		* @param mouseInput Reference to the MouseInput system.
		* @param keyboardInput Reference to the KeyboardInput system.
		*/
		void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) override;

		/**
		* @brief Sets the default camera position in the scene.
		* @param size Scene size or area to position the camera at.
		*/
		void SetDefaultCameraPosition(int size) override;

		/**
		* @brief Sets the camera's world position.
		* @param x X coordinate.
		* @param y Y coordinate.
		* @param z Z coordinate.
		*/
		void SetCameraPosition(float x, float y, float z) override;

		/**
		* @brief Returns the camera's view matrix for rendering.
		* @return Reference to the view matrix.
		*/
		glm::mat4& GetViewMatrix() override;

		/**
		* @brief Returns the camera's projection matrix for rendering.
		* @return Reference to the projection matrix.
		*/
		glm::mat4& GetProjectionMatrix() override;

		/**
		* @brief Returns the current world position of the camera.
		* @return 3D vector representing camera position.
		*/
		glm::vec3 GetCameraPosition() override;

		/**
		* @brief Checks whether the camera has spawned into the scene.
		* @return True if spawned, false otherwise.
		*/
		bool IsSpawned();

		/**
		* @brief Sets the spawned state of the camera.
		* @param spawned True to mark as spawned, false otherwise.
		*/
		void SetSpawned(bool spawned);

	protected:
		/**
		* @brief Handles keyboard input for movement (WASD, jump, etc.).
		* @param keyboardInput Reference to the keyboard input system.
		* @param dt Delta time since last frame.
		*/
		void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) override;

		/**
		* @brief Handles mouse input for looking around.
		* @param mouseInput Reference to the mouse input system.
		* @param dt Delta time since last frame.
		*/
		void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) override;

	private:
		/**
		* @brief Calculates the camera's forward vector from yaw and pitch.
		* Updates the PlayerCameraProperties::direction vector.
		*/
		void CalculateCameraForward();

		Transform m_transform;								/**< Transform storing position, rotation, scale. */
		CameraProperties m_cameraProperties;				/**< Common camera properties (projection, matrices, etc.). */
		PlayerCameraProperties m_playerCameraProperties;	/**< Player-specific camera properties (orientation, speed). */

		bool m_spawned{ false };							/**< Flag indicating whether the camera has spawned into the scene. */
	};
} // namespace pwg

#endif // !SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_
