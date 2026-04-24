#ifndef SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H
#define SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H

#include <unordered_map>
#include "graphics/shader/computeShader.h"
#include "graphics/shader/shaderManager.h"
#include "terrain/graph/terrainGraph.h"
#include "terrain/graph/passes/elevationPass.h"
#include "terrain/graph/passes/finalMaskPass.h"
#include "terrain/graph/passes/mountainMaskPass.h"
#include "terrain/graph/passes/mountainHeightPass.h"
#include "terrain/graph/passes/finalHeightPass.h"
#include "terrain/graph/passes/normalMaskPass.h"
#include "terrain/graph/passes/slopeMaskPass.h"
#include "terrain/chunk/terrainChunk.h"

#include "terrain/debug/terrainDebug.h"

namespace pwg
{
	class TerrainComputePipeline
	{
	public:
		TerrainComputePipeline(int terrainSize, std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<TerrainDebug> debug);
		~TerrainComputePipeline();

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);

		void BuildGraph();
		void AddComputeShader(std::string name, std::shared_ptr<ComputeShader> computeShader);
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> CreateMasks();
		void ClearTextures();
		std::shared_ptr<TerrainGraph> GetGraph();
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& GetMasks();
		int GetSize();

		std::shared_ptr<Texture> GetFinalMaskTexture();
		std::unordered_map<std::string, std::shared_ptr<Texture>>& GetFinalTerrainMasks();
	private:
		std::unordered_map<std::string, std::shared_ptr<ComputeShader>> m_computeShaders;
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_terrainMasks;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_finalTerrainMasks;
		std::shared_ptr<TerrainGraph> m_graph;
		std::shared_ptr<ShaderManager> m_shaderManager;
		std::shared_ptr<TerrainDebug> m_terrainDebug;

		TerrainPassContext m_terrainPassContext;
		int m_size{ 0 };
	};
}
#endif // !SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H
