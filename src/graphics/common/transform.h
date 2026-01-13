#ifndef SRC_GRAPHICS_COMMON_TRANSFORM_H_
#define SRC_GRAPHICS_COMMON_TRANSFORM_H_

#include <glm/vec3.hpp>

namespace pwg
{
	/**
	* @brief Structure representing a 3D transform.
	* Stores position, rotation, and scale vectors used for spatial transformations.
	*/
	struct Transform
	{
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };	/**< World-space position */
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };	/**< Rotation in radians (pitch, yaw, roll) */
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };	/**< Non-uniform scale factors */
	};
}

#endif // !SRC_GRAPHICS_COMMON_TRANSFORM_H_
