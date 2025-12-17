#include "terrainLayersManager.h"

namespace pwg
{
	TerrainLayersManager::TerrainLayersManager()
	{
		m_terrainLayers.push_back(TerrainLayer(0.0f, 0.45f, 0.0f, 1.0f, 1.0f, 0));
		m_terrainLayers.push_back(TerrainLayer(0.35f, 0.5f, 0.15f, 0.65f, 1.0f, 1));
		m_terrainLayers.push_back(TerrainLayer(0.35f, 0.85f, 0.3f, 1.0f, 1.0f, 2));
		m_terrainLayers.push_back(TerrainLayer(0.75f, 1.0f, 0.0f, 1.0f, 1.0f, 3));
	}

	std::vector<TerrainLayer> TerrainLayersManager::GetLayers()
	{
		return m_terrainLayers;
	}
}

