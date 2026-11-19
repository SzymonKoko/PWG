#ifndef SRC_TERRAIN_LOD_LOD_DISTANCES_H_
#define SRC_TERRAIN_LOD_LOD_DISTANCES_H_

#include <memory>

namespace pwg
{
	struct LODDistances
	{
		// These are the default distances. The live values are in LODManager::Settings
		// (runtime-settable from the Debug View, D-05) and start from these.
		// Distances are tuned so only the few chunks closest to the camera use the
		// highest detail meshes. Previously these thresholds were so large that
		// flying into the terrain promoted hundreds of chunks to LOD0 in a single
		// frame, which could stall/crash the GPU (device lost).
		std::pair<unsigned int, float> lod0{ 0, 220.0f };
		std::pair<unsigned int, float> lod1{ 1, 450.0f };
		std::pair<unsigned int, float> lod2{ 2, 900.0f };
		std::pair<unsigned int, float> lod3{ 3, 1600.0f };
		std::pair<unsigned int, float> lod4{ 4, 2600.0f };
	};
}
#endif // !SRC_TERRAIN_LOD_LOD_DISTANCES_H_
