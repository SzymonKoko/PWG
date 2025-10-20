#ifndef SRC_SCENE_COMPONENTS_CAMERA_COMPONENT
#define SRC_SCENE_COMPONENTS_CAMERA_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	/**
	 * @brief Component representing a camera in the scene.
	 * Stores camera properties such as FOV, near/far planes, aspect ratio, and transformation matrices.
	 */
	struct CameraComponent
	{
		static constexpr const char* Name = "Camera Component"; /**< Component name identifier. */

		float fov = 45.0f;						/**< Field of view in degrees. */
		float nearPlane = 0.01f;				/**< Near clipping plane distance. */
		float farPlane = 2000.0f;				/**< Far clipping plane distance. */
		float aspectRatio = 16.0f / 9.0f;		/**< Aspect ratio (width / height). */
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };	/**< Up direction vector for the camera. */

		glm::mat4 viewMatrix{ 1.0f };			/**< View matrix of the camera. */
		glm::mat4 projectionMatrix{ 1.0f };		/**< Projection matrix of the camera. */
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_CAMERA_COMPONENT
