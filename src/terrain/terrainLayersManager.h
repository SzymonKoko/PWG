#ifndef SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H
#define SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "graphics/texture/texture.h"
#include <string>

namespace pwg
{

	/**
	 * @brief Represents a single terrain layer used for coloring or texturing the terrain.
	 */
	struct TerrainLayer
	{
		int layerID;
		float minHeight;			/**< Minimum height at which this layer is applied. */
		float maxHeight;			/**< Maximum height at which this layer is applied. */
		float minSlope;				/**< Minimum slope at which this layer is applied. */
		float maxSlope;				/**< Maximum slope at which this layer is applied. */
		float blendStrength;
		int textureIndex;			/**< ID of the texture associated with this layer. */

		bool dirty = true;
	};

	class TerrainLayersManager
	{
	public:
		TerrainLayersManager();
		~TerrainLayersManager() = default;

		std::vector<TerrainLayer>& GetLayers();

	private:
		std::vector<TerrainLayer> m_terrainLayers;
		std::unordered_map<std::string, unsigned int> m_terrainTextureIndexes;

	};
}
#endif // !SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H
