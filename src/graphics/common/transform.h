#ifndef SRC_GRAPHICS_COMMON_TRANSFORM_H_
#define SRC_GRAPHICS_COMMON_TRANSFORM_H_

#include <glm/vec3.hpp>

namespace pwg
{
	struct Transform
	{
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	};
}
#endif // !SRC_GRAPHICS_COMMON_TRANSFORM_H_
