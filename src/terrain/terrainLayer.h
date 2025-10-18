#ifndef SRC_TERRAIN_TERRAIN_LAYER_H
#define SRC_TERRAIN_TERRAIN_LAYER_H

#include <string>
#include <glm/vec3.hpp>

namespace pwg
{
	struct TerrainLayer
	{
		bool enabled;
		std::string name;
		float minHeight;
		float maxHeight;
		unsigned int textureID;
		glm::vec3 color;
	};
}

#endif // !SRC_TERRAIN_TERRAIN_LAYER_H
