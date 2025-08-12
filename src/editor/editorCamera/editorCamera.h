#ifndef SRC_EDITOR_EDITOR_CAMERA_EDITOR_CAMERA_H
#define SRC_EDITOR_EDITOR_CAMERA_EDITOR_CAMERA_H

#include "graphics/camera/icamera.h"
#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"

namespace pwg
{
	class EditorCamera : public ICamera
	{
	public:
		EditorCamera(GLFWwindow* window, MouseInput* mimput, KeyboardInput* kinput);
		~EditorCamera() = default;

		void Update() override;

		void UpdateViewMatrix() override;
		void UpdateProjectionMatrix(float aspectRatio) override;
		void SetPosition(const glm::vec3& position) override;
		glm::vec3& GetPosition() override;

		void HandleMouseInput();
		void HandleKeyboardInput();
		void HandleOrbit();
		void HandlePan();
		void HandleZoom(double scroll);

	private:
		GLFWwindow* m_window;
		MouseInput* m_mouseInput;
		KeyboardInput* m_keyboardInput;

		float m_yaw;
		float m_pitch;
		float m_distanceToTarget;
		glm::vec3 m_target;

		float m_minZoom{ 5.0f };
		float m_maxZoom{ 500.0f };
		float m_zoomSpeed{ 5.0f };

	};
}
#endif // !SRC_EDITOR_EDITOR_CAMERA_EDITOR_CAMERA_H
