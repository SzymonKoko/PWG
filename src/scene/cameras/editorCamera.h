#ifndef SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_
#define SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_

#include "icamera.h"
#include "graphics/common/transform.h"

namespace pwg
{
	/**
	* @brief Struct storing specific properties for the editor camera.
	* Includes rotation angles, distance to target, zoom limits, and target position.
	*/
	struct EditorCameraProperties
	{
		float yaw{ -45.0f };					/**< Yaw rotation angle in degrees. */
		float pitch{ 45.0f };					/**< Pitch rotation angle in degrees. */
		float distanceToTarget{ 200.0f };		/**< Distance from the camera to the target point. */
		glm::vec3 target{ 0.0f, 0.0f, 0.0f };	/**< Target point the camera is looking at. */

		float minZoom{ 5.0f };					/**< Minimum zoom distance. */
		float maxZoom{ 700.0f };				/**< Maximum zoom distance. */
		float zoomSpeed{ 50.0f };				/**< Speed factor for zooming in/out. */
	};

	/**
	* @brief Editor camera class for scene inspection.
	* Inherits from ICamera and supports orbiting, zooming, and free movement around a target.
	*/
	class EditorCamera : public ICamera
	{
	public:
		/**
		* @brief Constructs an editor camera with default properties.
		*/
		EditorCamera();

		/**
		* @brief Destructor. Cleans up camera resources.
		*/
		~EditorCamera() = default;

		/**
		* @brief Updates the camera each frame using mouse and keyboard input.
		* @param dt Delta time since last frame.
		* @param mouseInput Reference to the MouseInput system.
		* @param keyboardInput Reference to the KeyboardInput system.
		*/
		void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) override;

		/**
		* @brief Sets the default camera position based on scene size.
		* @param size Size of the scene or area to position camera at.
		*/
		void SetDefaultCameraPosition(int size) override;

		/**
		* @brief Sets the camera position in world space.
		* @param x X coordinate.
		* @param y Y coordinate.
		* @param z Z coordinate.
		*/
		void SetCameraPosition(float x, float y, float z) override;

		/**
		* @brief Returns the current view matrix for rendering.
		* @return Reference to the 4x4 view matrix.
		*/
		glm::mat4& GetViewMatrix() override;

		/**
		* @brief Returns the current projection matrix for rendering.
		* @return Reference to the 4x4 projection matrix.
		*/
		glm::mat4& GetProjectionMatrix() override;

		/**
		* @brief Returns the current world position of the camera.
		* @return 3D vector representing camera position.
		*/
		glm::vec3 GetCameraPosition() override;

	protected:
		/**
		* @brief Processes keyboard input to move or rotate the camera.
		* @param keyboardInput Reference to KeyboardInput system.
		* @param dt Delta time since last frame.
		*/
		void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) override;

		/**
		* @brief Processes mouse input to orbit, pan, or zoom the camera.
		* @param mouseInput Reference to MouseInput system.
		* @param dt Delta time since last frame.
		*/
		void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) override;

	private:
		/**
		* @brief Calculates the forward direction of the camera based on yaw and pitch.
		*/
		void CalculateCameraForward();

		Transform m_transform;								/**< Camera transform (position, rotation, scale). */
		CameraProperties m_cameraProperties;				/**< Base camera properties from ICamera. */
		EditorCameraProperties m_editorCameraProperties;	/**< Editor-specific properties (orbit, zoom, target). */
	};

} // namespace pwg

#endif // !SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_
