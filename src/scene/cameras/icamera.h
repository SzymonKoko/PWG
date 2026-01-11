#ifndef SRC_SCENE_CAMERAS_ICAMERA_H_
#define SRC_SCENE_CAMERAS_ICAMERA_H_

#include "core/input/keyboardInput.h"
#include "core/input/mouseInput.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace pwg
{
	struct CameraProperties
	{
		float fov = 60.0f;						/**< Field of view in degrees. */
		float nearPlane = 0.01f;				/**< Near clipping plane distance. */
		float farPlane = 20000.0f;				/**< Far clipping plane distance. */
		float aspectRatio = 16.0f / 9.0f;		/**< Aspect ratio (width / height). */
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };	/**< Up direction vector for the camera. */
		glm::vec3 forward = { 0.0f, 0.0f, -1.0f };

		glm::mat4 viewMatrix{ 1.0f };			/**< View matrix of the camera. */
		glm::mat4 projectionMatrix{ 1.0f };		/**< Projection matrix of the camera. */
	};

	class ICamera
	{
	public:
		virtual ~ICamera() = default;
		virtual void Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput) = 0;

		virtual void SetDefaultCameraPosition(int size) = 0;
		virtual void SetCameraPosition(float x, float y, float z) = 0;

		virtual glm::mat4& GetViewMatrix() = 0;
		virtual glm::mat4& GetProjectionMatrix() = 0;

	protected:

		virtual void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt) = 0;
		virtual void HandleMouseInput(pwg::MouseInput& mouseInput, float dt) = 0;
	};

}
#endif // !SRC_SCENE_CAMERAS_ICAMERA_H_
