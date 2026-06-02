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

	void TerrainGraph::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		ExecutePasses(TerrainPassTypes::GENERATION, masks, ctx);
		ExecutePasses(TerrainPassTypes::MASK, masks, ctx);
	}

	void TerrainGraph::ExecutePasses(TerrainPassTypes passType, std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		for (const auto& pass : m_passes)
		{
			if (pass->GetPassType() == passType)
			{
				pass->Execute(masks, ctx);
			}
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

