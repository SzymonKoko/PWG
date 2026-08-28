#ifndef SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H
#define SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H

#include <unordered_map>
#include <cstdint>
#include <string>
#include "graphics/shader/computeShader.h"
#include "graphics/shader/shaderManager.h"
#include "terrain/graph/terrainGraph.h"
#include "terrain/graph/universalTerrainGraph.h"
#include "terrain/graph/passes/elevationPass.h"
#include "terrain/graph/passes/finalMaskPass.h"
#include "terrain/graph/passes/mountainMaskPass.h"
#include "terrain/graph/passes/mountainHeightPass.h"
#include "terrain/graph/passes/finalHeightPass.h"
#include "terrain/graph/passes/normalMaskPass.h"
#include "terrain/graph/passes/slopeMaskPass.h"
#include "terrain/graph/passes/hydraulicFluxPass.h"
#include "terrain/graph/passes/hydraulicWaterPass.h"
#include "terrain/graph/passes/erosionPass.h"
#include "terrain/graph/passes/particleErosionPass.h"
#include "terrain/chunk/terrainChunk.h"

#include "terrain/debug/terrainDebug.h"

namespace pwg
{
	struct VegetationLayerDescriptor
	{
		int nodeId{ 0 };
		std::string assetId{ "builtin.tree_a" };
		int seed{ 1337 };
		float density{ 0.5f };
		float spacing{ 50.0f };
		float minScale{ 0.8f };
		float maxScale{ 1.2f };
		float maxSlopeDegrees{ 30.0f };
		bool alignToSlope{ false };
		float maxTiltDegrees{ 30.0f };
		float renderDistance{ 1000.0f };
		std::shared_ptr<TerrainMask> densityMask;
	};

	class TerrainComputePipeline
	{
	public:
		TerrainComputePipeline(int terrainSize, std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<TerrainDebug> debug);
		~TerrainComputePipeline();

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void RegenerateTerrain(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void StepSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void StepWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void SettleWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, int iterations);
		void StepErosionSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void StepParticleBake(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool uploadPreview);
		void BakeParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void RefreshMaskPass(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void GenerateWorld(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void ResetWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void ResetErosionSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool ExecuteUniversalGraph(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void ApplyUniversalGraphSimulationSettings();

		TerrainPassContext& GetPassContext();

		void BuildGraph();
		void AddComputeShader(std::string name, std::shared_ptr<ComputeShader> computeShader);
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> CreateMasks();
		void ClearTextures();
		std::shared_ptr<TerrainGraph> GetGraph();
		TerrainGraphAsset& GetUniversalGraphAsset();
		const TerrainGraphAsset& GetUniversalGraphAsset() const;
		const std::vector<VegetationLayerDescriptor>& GetVegetationLayers() const;
		std::uint64_t GetVegetationRevision() const;
		void ResetUniversalGraphToDefault();
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& GetMasks();
		HydraulicSimulationSettings& GetHydraulicSettings();
		int GetSize();

		std::shared_ptr<Texture> GetFinalMaskTexture();
		std::unordered_map<std::string, std::shared_ptr<Texture>>& GetFinalTerrainMasks();
	private:
		std::unordered_map<std::string, std::shared_ptr<ComputeShader>> m_computeShaders;
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_terrainMasks;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_finalTerrainMasks;
		std::shared_ptr<TerrainGraph> m_graph;
		TerrainGraphAsset m_universalGraphAsset;
		std::vector<VegetationLayerDescriptor> m_vegetationLayers;
		std::uint64_t m_vegetationRevision{ 0 };
		std::shared_ptr<ShaderManager> m_shaderManager;
		std::shared_ptr<TerrainDebug> m_terrainDebug;

		TerrainPassContext m_terrainPassContext;
		int m_size{ 0 };
	};
}
#endif // !SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H
