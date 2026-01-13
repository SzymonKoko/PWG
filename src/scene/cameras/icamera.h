#ifndef SRC_SCENE_CAMERAS_ICAMERA_H_
#define SRC_SCENE_CAMERAS_ICAMERA_H_

#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace pwg
{
	/**
	* @brief Struct storing common camera properties.
	* Used by all camera types to manage projection, orientation, and matrices.
	*/
	struct CameraProperties
	{
		float fov = 60.0f;							/**< Field of view in degrees. */
		float nearPlane = 0.01f;					/**< Near clipping plane distance. */
		float farPlane = 20000.0f;					/**< Far clipping plane distance. */
		float aspectRatio = 16.0f / 9.0f;			/**< Aspect ratio (width / height). */
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };		/**< Up direction vector for the camera. */
		glm::vec3 forward = { 0.0f, 0.0f, -1.0f };	/**< Forward direction vector of the camera. */

		glm::mat4 viewMatrix{ 1.0f };				/**< View matrix of the camera. */
		glm::mat4 projectionMatrix{ 1.0f };			/**< Projection matrix of the camera. */
	};

	/**
	* @brief Abstract camera interface.
	* Defines the basic functions all camera types must implement.
	* Supports updating, setting positions, and retrieving view/projection matrices.
	*/
	class ICamera
	{
	public:
		/**
		* @brief Virtual destructor for proper cleanup of derived camera types.
		*/
		virtual ~ICamera() = default;

		/**
		* @brief Updates the camera each frame using input.
		* @param dt Delta time since last frame.
		* @param mouseInput Reference to the MouseInput system.
		* @param keyboardInput Reference to the KeyboardInput system.
		*/
		virtual void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) = 0;

		/**
		* @brief Sets the default camera position based on scene size.
		* @param size Size of the scene or area to position camera at.
		*/
		virtual void SetDefaultCameraPosition(int size) = 0;

		/**
		* @brief Sets the camera position in world space.
		* @param x X coordinate.
		* @param y Y coordinate.
		* @param z Z coordinate.
		*/
		virtual void SetCameraPosition(float x, float y, float z) = 0;

		/**
		* @brief Returns the current view matrix for rendering.
		* @return Reference to the 4x4 view matrix.
		*/
		virtual glm::mat4& GetViewMatrix() = 0;

		/**
		* @brief Returns the current projection matrix for rendering.
		* @return Reference to the 4x4 projection matrix.
		*/
		virtual glm::mat4& GetProjectionMatrix() = 0;

		/**
		* @brief Returns the current world position of the camera.
		* @return 3D vector representing camera position.
		*/
		virtual glm::vec3 GetCameraPosition() = 0;

	protected:
		/**
		* @brief Handles keyboard input for camera movement or rotation.
		* Must be implemented by derived camera classes.
		* @param keyboardInput Reference to KeyboardInput system.
		* @param dt Delta time since last frame.
		*/
		virtual void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) = 0;

		/**
		* @brief Handles mouse input for camera orbit, pan, or zoom.
		* Must be implemented by derived camera classes.
		* @param mouseInput Reference to MouseInput system.
		* @param dt Delta time since last frame.
		*/
		virtual void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) = 0;
	};

} // namespace pwg

#endif // !SRC_SCENE_CAMERAS_ICAMERA_H_
