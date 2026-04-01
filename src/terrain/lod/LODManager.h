#ifndef SRC_TERRAIN_LOD_LOD_MANAGER_H_
#define SRC_TERRAIN_LOD_LOD_MANAGER_H_

#include <memory>

#include "terrain/lod/LODDistances.h"
#include "graphics/mesh/meshManager.h"
#include "terrain/chunk/terrainChunk.h"

#define MAX_LOD 5

namespace pwg
{
	class LODManager
	{
	public:
		LODManager(int chunkSize);
		~LODManager() = default;

		void GenerateLODMeshes();
		void SelectLOD(TerrainChunk& chunk, float distance);

		std::shared_ptr<Mesh> GetMesh(int lod);
	private:
		std::shared_ptr<Mesh> m_lodMeshes[MAX_LOD];
		LODDistances m_lodDistances;
		int m_chunkSize{ 0 };
	};
}
#endif // !SRC_TERRAIN_LOD_LOD_MANAGER_H_
