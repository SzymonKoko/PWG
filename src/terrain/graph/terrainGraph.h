#ifndef SRC_TERRAIN_GRAPH_TERRAIN_GRAPH_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_GRAPH_H_

#include <string>
#include "passes/terrainPass.h"

namespace pwg
{
	struct TerrainPassContext;

	class TerrainGraph
	{
	public:
		TerrainGraph();
		~TerrainGraph();

		void AddPass(std::shared_ptr<TerrainPass> pass);
		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx);
		std::vector<std::shared_ptr<TerrainPass>>& GetPasses();
		void ClearGraph();

	private:
		std::vector<std::shared_ptr<TerrainPass>> m_passes;

	};
}

#endif // !SRC_TERRAIN_GRAPH_TERRAIN_GRAPH_H_
