#include "terrainGraph.h"

namespace pwg
{
	TerrainGraph::TerrainGraph()
	{
		m_passes.clear();
	}

	TerrainGraph::~TerrainGraph()
	{
	}

	void TerrainGraph::AddPass(std::shared_ptr<TerrainPass> pass)
	{
		m_passes.push_back(pass);
	}

	void TerrainGraph::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		for (const auto& pass : m_passes)
		{
			pass->Execute(masks);
		}
	}
	std::vector<std::shared_ptr<TerrainPass>>& TerrainGraph::GetPasses()
	{
		return m_passes;
	}
	void TerrainGraph::ClearGraph()
	{
		m_passes.clear();
	}
}

