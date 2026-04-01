#include "LODManager.h"
#include "glm/glm.hpp"

namespace pwg
{
	LODManager::LODManager(int chunkSize)
		: m_chunkSize(chunkSize)
	{

	}
	void LODManager::GenerateLODMeshes()
	{
		m_lodMeshes[0] = MeshManager::CreatePlaneMesh(m_chunkSize, 0);
		m_lodMeshes[1] = MeshManager::CreatePlaneMesh(m_chunkSize, 1);
		m_lodMeshes[2] = MeshManager::CreatePlaneMesh(m_chunkSize, 2);
		m_lodMeshes[3] = MeshManager::CreatePlaneMesh(m_chunkSize, 3);
		m_lodMeshes[4] = MeshManager::CreatePlaneMesh(m_chunkSize, 4);
	}
	void LODManager::SelectLOD(TerrainChunk& chunk, float distance)
	{
		if (distance <= m_lodDistances.lod0.second)
		{
			chunk.lod = m_lodDistances.lod0.first;
		}
		else if (distance <= m_lodDistances.lod1.second)
		{
			chunk.lod = m_lodDistances.lod1.first;
		}
		else if (distance <= m_lodDistances.lod2.second)
		{
			chunk.lod = m_lodDistances.lod2.first;
		}
		else if (distance <= m_lodDistances.lod3.second)
		{
			chunk.lod = m_lodDistances.lod3.first;
		}
		else 
		{
			chunk.lod = m_lodDistances.lod4.first;
		}
		
	}
	std::shared_ptr<Mesh> LODManager::GetMesh(int lod)
	{
		lod = glm::clamp(lod, 0, MAX_LOD - 1);
		return m_lodMeshes[lod];
	}
}

