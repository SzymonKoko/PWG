#ifndef SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_
#define SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_

#include "icamera.h"
#include "graphics/common/transform.h"

namespace pwg
{
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

	class EditorCamera : public ICamera
	{
	public:
		EditorCamera();
		~EditorCamera() = default;

		void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) override;

		void SetDefaultCameraPosition(int size) override;
		void SetCameraPosition(float x, float y, float z) override;

		glm::mat4& GetViewMatrix() override;
		glm::mat4& GetProjectionMatrix() override;
	protected:
		void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) override;
		void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) override;

	private:
		void CalculateCameraForward();

		Transform m_transform;
		CameraProperties m_cameraProperties;
		EditorCameraProperties m_editorCameraProperties;
	};

}
#endif // !SRC_SCENE_CAMERAS_EDITOR_CAMERA_H_
