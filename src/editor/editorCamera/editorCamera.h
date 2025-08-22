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

		/**
		* @brief Editor camera constructor. Constructs camera with given GlfWwindow, mouse and keyboard inputs
		* @param window Pointer to a GLFWwindow instance
		* @param minput Reference to a mouse input instance
		* @param kinput Reference to a keyboard input instance
		*/
		EditorCamera(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput);

		/**
		* @brief Editor camera destructor. Does nothing.
		*/
		~EditorCamera() = default;

		/**
		* @brief Updates camera on every frame. Handles inputs.
		*/
		void Update() override;

		/**
		* @brief Updates view matrix of a camera
		*/
		void UpdateViewMatrix() override;

		/**
		* @brief Updates projection matrix of a camera
		* @param aspectRatio New aspect ratio of a camera
		*/
		void UpdateProjectionMatrix(float aspectRatio) override;

		/**
		* @brief Camera position setter.
		* @param position Reference to a new position vector3 
		*/
		void SetPosition(const glm::vec3& position) override;

		/**
		* @brief Camera position getter
		* @return Returns a camera position vector3
		*/
		glm::vec3& GetPosition() override;

		/**
		* @brief Handles mouse input. Triggered on scroll or mouse move 
		*/
		void HandleMouseInput();

		/**
		* @brief Handles keyboard input
		*/
		void HandleKeyboardInput();

		/**
		* @brief Handles orbit on right mouse button click
		*/
		void HandleOrbit();

		/**
		* @brief Handles pan on middle mouse button click
		*/
		void HandlePan();

		/**
		* @brief Handles zoom on scroll move
		*/
		void HandleZoom(double scroll);

	private:
		GLFWwindow* m_window;
		MouseInput& m_mouseInput;
		KeyboardInput& m_keyboardInput;

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
