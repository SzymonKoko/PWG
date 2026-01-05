#ifndef SRC_SCENE_LIGHTS_LIGHT_H_
#define SRC_SCENE_LIGHTS_LIGHT_H_

#include <glm/vec3.hpp>

namespace pwg
{
	struct Light
	{
		glm::vec3 direction{ 0.0f };
		glm::vec3 color{ 0.0f };
		float intensivity{ 0.0f };
	};
}
#endif // !SRC_SCENE_LIGHTS_LIGHT_H_
