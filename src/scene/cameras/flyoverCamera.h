#ifndef SRC_SCENE_CAMERAS_FLYOVER_CAMERA_H_
#define SRC_SCENE_CAMERAS_FLYOVER_CAMERA_H_

#include "icamera.h"
#include "graphics/common/transform.h"

namespace pwg
{
	struct FlyoverCameraProperties
	{
		float yaw{ -45.0f };
		float pitch{ -20.0f };
		float sensitivity{ 0.1f };
		float acceleration{ 220.0f };
		float flightDrag{ 0.8f };
		float braking{ 3.5f };
		float maxSpeed{ 260.0f };
		float boostMultiplier{ 3.0f };
		float lookSmoothing{ 12.0f };
		float gimbalStrengthDegrees{ 4.0f };
		float gimbalSmoothing{ 6.0f };
		glm::vec3 velocity{ 0.0f };
	};

	class FlyoverCamera : public ICamera
	{
	public:
		FlyoverCamera();
		~FlyoverCamera() = default;

		void Update(float dt, MouseInput& mouseInput, KeyboardInput& keyboardInput) override;
		void SetDefaultCameraPosition(int size) override;
		void SetCameraPosition(float x, float y, float z) override;
		void SetAspectRatio(float aspectRatio) override;
		glm::mat4& GetViewMatrix() override;
		glm::mat4& GetProjectionMatrix() override;
		glm::vec3 GetCameraPosition() override;
		CameraProperties& GetCameraProperties() { return m_cameraProperties; }
		FlyoverCameraProperties& GetProperties() { return m_flyoverProperties; }

	protected:
		void HandleKeyboardInput(KeyboardInput& keyboardInput, float dt) override;
		void HandleMouseInput(MouseInput& mouseInput, float dt) override;

	private:
		void CalculateCameraForward();
		void UpdateMatrices();

		Transform m_transform;
		CameraProperties m_cameraProperties;
		FlyoverCameraProperties m_flyoverProperties;
		float m_targetYaw{ -45.0f };
		float m_targetPitch{ -20.0f };
		float m_gimbalRoll{ 0.0f };
		float m_mouseTurn{ 0.0f };
	};
}

#endif
