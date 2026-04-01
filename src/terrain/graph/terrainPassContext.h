#ifndef SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_

#include <glm/vec2.hpp>

namespace pwg
{
	struct TerrainPassContext
	{
		int chunkSize;
		glm::vec2 offset{ 0.0f, 0.0f };

	};
}
#endif // !SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
