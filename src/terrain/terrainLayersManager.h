#ifndef SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H
#define SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "graphics/texture/texture.h"

namespace pwg
{
	/**
	 * @brief Represents a single terrain layer used for coloring or texturing the terrain.
	 */
	struct TerrainLayer
	{
		int layerID;                /**< Unique identifier for the terrain layer. */
		float minHeight;            /**< Minimum height at which this layer is applied. */
		float maxHeight;            /**< Maximum height at which this layer is applied. */
		float minSlope;             /**< Minimum slope at which this layer is applied. */
		float maxSlope;             /**< Maximum slope at which this layer is applied. */
		float blendStrength;        /**< Strength of blending with other layers. */
		int textureIndex;           /**< Index of the texture associated with this layer. */
		bool dirty{ true };         /**< Flag indicating if the layer has been modified. */
	};

	/**
	 * @brief Manages multiple terrain layers and their associated textures.
	 * Handles retrieval, indexing, and organization of terrain layers.
	 */
	class TerrainLayersManager
	{
	public:

		/**
		 * @brief Default constructor.
		 */
		TerrainLayersManager();

		/**
		 * @brief Default destructor.
		 */
		~TerrainLayersManager() = default;

		/**
		 * @brief Returns a reference to the list of terrain layers.
		 * @return Vector of TerrainLayer objects.
		 */
		std::vector<TerrainLayer>& GetLayers();

	private:
		std::vector<TerrainLayer> m_terrainLayers;								/**< Container for all terrain layers. */
		std::unordered_map<std::string, unsigned int> m_terrainTextureIndexes;	/**< Maps texture names/IDs to their indexes for layer usage. */
	};
} // namespace pwg

#endif // !SRC_TERRAIN_TERRAIN_LAYERS_MANAGER_H
