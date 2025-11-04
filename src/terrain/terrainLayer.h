#ifndef SRC_TERRAIN_TERRAIN_LAYER_H
#define SRC_TERRAIN_TERRAIN_LAYER_H

#include <string>
#include <glm/vec3.hpp>

namespace pwg
{
	/**
	 * @brief Represents a single terrain layer used for coloring or texturing the terrain.
	 */
	struct TerrainLayer
	{
		bool enabled;				/**< Determines whether the layer is active. */
		std::string name;			/**< Name of the terrain layer. */
		unsigned int layerLevel;	/**< Level of layer in hierarchy */
		float minHeight;			/**< Minimum height at which this layer is applied. */
		float maxHeight;			/**< Maximum height at which this layer is applied. */
		unsigned int textureID;		/**< ID of the texture associated with this layer. */
		glm::vec3 color;			/**< Color of the terrain layer (used if no texture). */

		inline void SetStatus(bool enabled) { enabled = enabled; }
	};
}

#endif // !SRC_TERRAIN_TERRAIN_LAYER_H
