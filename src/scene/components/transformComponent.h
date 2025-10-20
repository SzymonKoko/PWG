#ifndef SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT
#define SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	/**
	 * @brief Component that stores transformation data for an entity.
	 * Includes position and rotation vectors.
	 */
	struct TransformComponent
	{
		static constexpr const char* Name = "Transform Component"; /**< Component name identifier. */

		glm::vec3 position{ 0.0f, 0.0f, 0.0f }; /**< Position of the entity in world space. */
		glm::vec3 rotation{ 1.0f, 0.0f, 0.0f }; /**< Rotation vector (Euler angles) of the entity. */
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_TRANSFORM_COMPONENT
