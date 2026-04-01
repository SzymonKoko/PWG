#ifndef SRC_TERRAIN_LOD_LOD_DISTANCES_H_
#define SRC_TERRAIN_LOD_LOD_DISTANCES_H_

#include <memory>

namespace pwg
{
	struct LODDistances
	{
		std::pair<unsigned int, float> lod0{ 0, 1100.0f };
		std::pair<unsigned int, float> lod1{ 1, 1400.0f };
		std::pair<unsigned int, float> lod2{ 2, 1800.0f };
		std::pair<unsigned int, float> lod3{ 3, 2200.0f };
		std::pair<unsigned int, float> lod4{ 4, 2600.0f };
	};
}
#endif // !SRC_TERRAIN_LOD_LOD_DISTANCES_H_
