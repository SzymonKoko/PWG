#ifndef SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_
#define SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_

#include "icamera.h"
#include "graphics/common/transform.h"

namespace pwg
{
	struct PlayerCameraProperties
	{
		float yaw{ -90.0f };					/**< Yaw rotation angle in degrees. */
		float pitch{ 90.0f };					/**< Pitch rotation angle in degrees. */
		glm::vec3 direction{ 0.0f ,0.0f, 0.0f };

		float cameraSensitivity{ 0.1f };
		float cameraSpeed{ 0.25f };
	};

	class PlayerCamera : public ICamera
	{
	public:
		PlayerCamera();
		~PlayerCamera() = default;

		void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) override;

		void SetDefaultCameraPosition(int size) override;
		void SetCameraPosition(float x, float y, float z) override;

		glm::mat4& GetViewMatrix() override;
		glm::mat4& GetProjectionMatrix() override;

		bool IsSpawned();
		void SetSpawned(bool spawned);

	protected:
		void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) override;
		void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) override;

	private:

		void CalculateCameraForward();

		Transform m_transform;
		CameraProperties m_cameraProperties;
		PlayerCameraProperties m_playerCameraProperties;

		bool m_spawned{ false };
	};
}
#endif // !SRC_SCENE_CAMERAS_PLAYER_CAMERA_H_
