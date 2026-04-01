#ifndef SRC_TERRAIN_CHUNK_TERRAIN_CHUNK_H_
#define SRC_TERRAIN_CHUNK_TERRAIN_CHUNK_H_

#include <glm/vec2.hpp>
#include "graphics/mesh/mesh.h"
#include "graphics/material/material.h"
#include "terrain/masks/mask.h"

namespace pwg
{
	struct TerrainChunk
	{
		glm::ivec2 chunkCoords;
		std::shared_ptr<Material> material;

		std::unordered_map<std::string, std::shared_ptr<Texture>> masks;
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> localMasks;

		int lod;
		bool generated{ false };

	};
}
#endif // !SRC_TERRAIN_CHUNK_TERRAIN_CHUNK_H_
