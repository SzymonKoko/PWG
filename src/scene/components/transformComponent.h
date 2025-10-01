#ifndef SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT
#define SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	struct TransformComponent
	{
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 1.0f, 0.0f, 0.0f };
	};
}
#endif // !SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT
