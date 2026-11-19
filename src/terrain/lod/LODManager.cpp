#include "LODManager.h"
#include "glm/glm.hpp"
#include <cmath>

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
		if (!m_settings.enabled)
		{
			chunk.lod = m_lodDistances.lod0.first;
			return;
		}

		// With the default scale of 1 the products are exactly the unscaled distances.
		const float scale = m_settings.scale;
		const Thresholds& d = m_settings.distances;
		if (distance <= d[0] * scale)
		{
			chunk.lod = m_lodDistances.lod0.first;
		}
		else if (distance <= d[1] * scale)
		{
			chunk.lod = m_lodDistances.lod1.first;
		}
		else if (distance <= d[2] * scale)
		{
			chunk.lod = m_lodDistances.lod2.first;
		}
		else if (distance <= d[3] * scale)
		{
			chunk.lod = m_lodDistances.lod3.first;
		}
		else 
		{
			chunk.lod = m_lodDistances.lod4.first;
		}
		
	}
	bool LODManager::SetScale(float scale)
	{
		if (!std::isfinite(scale) || scale <= 0.0f || scale > MaxScale)
		{
			return false;
		}
		m_settings.scale = scale;
		return true;
	}
	bool LODManager::SetDistances(const Thresholds& distances)
	{
		float previous = 0.0f;
		for (float value : distances)
		{
			if (!std::isfinite(value) || value < previous)
			{
				return false;
			}
			previous = value;
		}
		m_settings.distances = distances;
		return true;
	}
	std::shared_ptr<Mesh> LODManager::GetMesh(int lod)
	{
		lod = glm::clamp(lod, 0, MAX_LOD - 1);
		return m_lodMeshes[lod];
	}
}

