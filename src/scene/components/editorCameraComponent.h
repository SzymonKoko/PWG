#ifndef SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT
#define SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	struct EditorCameraComponent
	{
		float yaw{ -45.0f };
		float pitch{ 45.0f };
		float distanceToTarget{ 200.0f };
		glm::vec3 target{ -16.0f, 0.0f, 16.0f };

		float minZoom{ 5.0f };
		float maxZoom{ 500.0f };
		float zoomSpeed{ 5.0f };
	};
}
#endif // !SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT
