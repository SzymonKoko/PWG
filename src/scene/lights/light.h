#ifndef SRC_SCENE_LIGHTS_LIGHT_H_
#define SRC_SCENE_LIGHTS_LIGHT_H_

#include <glm/vec3.hpp>

namespace pwg
{
	/**
	 * @brief Struct representing a simple directional light.
	 * Contains direction, color, and intensity of the light.
	 */
	struct Light
	{
		glm::vec3 position{ 0.0f };
		glm::vec3 direction{ 0.0f }; /**< Direction the light is pointing to. */
		glm::vec3 color{ 0.0f };     /**< RGB color of the light. */
		float intensivity{ 0.0f };   /**< Strength or intensity of the light. */
		float exposure{ 1.0f };
		float skyAmbient{ 0.08f };
		float groundAmbient{ 0.025f };
		float normalStrength{ 0.65f };
		float snowHeight{ 1800.0f };
		float snowSoftness{ 850.0f };
		float wetnessStrength{ 0.65f };
		float waterAlpha{ 0.55f };
	};
} // namespace pwg

#endif // !SRC_SCENE_LIGHTS_LIGHT_H_
