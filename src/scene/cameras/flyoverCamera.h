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

		/** @brief Largest absolute pitch the camera can reach (the mouse clamp and SetPose), in degrees. */
		static constexpr float MaxPitchDegrees = 89.0f;

		/** @brief Yaw in degrees (the smoothed value that the view matrix uses; not wrapped). */
		float GetYaw() const { return m_flyoverProperties.yaw; }
		/** @brief Pitch in degrees. */
		float GetPitch() const { return m_flyoverProperties.pitch; }
		/** @brief Gimbal roll in radians. */
		float GetRoll() const { return m_gimbalRoll; }

		/**
		* @brief Sets the whole pose at once and rebuilds the matrices (camera path playback, D-10).
		*
		* Yaw and pitch are set together with their targets, so look smoothing has nothing left to catch up on, and
		* velocity and mouse turn are cleared, so the camera does not move on after the pose is set. Pitch is
		* clamped to +-MaxPitchDegrees, the range the mouse can reach; every pitch in it, including 0, is valid.
		* Update must not be called in the same frame: it would integrate input and dt on top of the pose.
		* The matrices use the aspect ratio, fov and clip planes the camera has at the time of the call.
		*/
		void SetPose(const glm::vec3& position, float yawDegrees, float pitchDegrees, float rollRadians);

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
