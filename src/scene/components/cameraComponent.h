#ifndef SRC_SCENE_COMPONENTS_CAMERA_COMPONENT
#define SRC_SCENE_COMPONENTS_CAMERA_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	struct CameraComponent
	{
		float fov = 45.0f;
		float nearPlane = 0.01;
		float farPlane = 2000.0f;
		float aspectRatio = 16.0f / 9.0f;
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };

		glm::mat4 viewMatrix{ 1.0f };
		glm::mat4 projectionMatrix{ 1.0f };
	};
}
#endif // !SRC_SCENE_COMPONENTS_CAMERA_COMPONENT
