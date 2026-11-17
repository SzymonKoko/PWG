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
#include "terrain/erosion/gpuParticleErosion.h"
#include "terrain/erosion/gpuThermalErosion.h"
#include "terrain/water/waterClipmapRenderer.h"
#include "terrain/hydrology/drainageNetworkGenerator.h"
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
		void RefreshMaskPass(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void GenerateWorld(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool ExecuteUniversalGraph(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool RefreshProceduralAssets(const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		const std::string& GetProceduralAssetStatus() const;
		void InvalidateWaterRenderCache();
		void DrawWaterClipmap(const std::shared_ptr<Shader>& shader, const glm::vec3& cameraPosition,
			const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool GenerateHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool UpdateHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void CancelHydrology();
		bool ResetHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool IsHydrologyRunning() const;
		float GetHydrologyProgress() const;
		std::string GetHydrologyStatus() const;
		void MarkHydrologyRestored(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool restored);
		bool UpdateParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, float deltaTime);
		bool StepParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool ResetParticleErosionState(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void StartParticleErosion();
		void PauseParticleErosion();
		void CancelParticleErosion();
		bool IsParticleErosionActive() const;
		bool IsParticleErosionReady() const;
		float GetParticleErosionSpeed() const;
		void SetParticleErosionSpeed(float speed);
		bool GetShowErosionParticles() const;
		void SetShowErosionParticles(bool show);
		int GetActiveParticleCount() const;
		void DrawErosionParticles(const glm::mat4& view, const glm::mat4& projection);
		bool UpdateThermalErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, float deltaTime);
		bool StepThermalErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool ResetThermalErosionState(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		bool StartThermalErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void PauseThermalErosion();
		bool IsThermalErosionActive() const;
		bool IsThermalErosionReady() const;
		bool IsThermalErosionStable() const;
		bool HasThermalErosionState() const;
		float GetThermalErosionSpeed() const;
		void SetThermalErosionSpeed(float speed);
		std::string GetThermalErosionStatus() const;
		void MarkThermalErosionRestored(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool restored);
		void ApplyUniversalGraphSimulationSettings();

		/** @brief Read only, for the mask export sidecar (D-08): steps since the erosion state was last reset. */
		unsigned int GetParticleStepCount() const { return m_particleStepCount; }
		unsigned int GetThermalStepCount() const { return m_thermalStepCount; }
		bool IsHydrologyGenerated() const { return m_hydrologyGenerated; }

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
		ThermalSimulationSettings& GetThermalSettings();
		int GetSize();

		std::shared_ptr<Texture> GetFinalMaskTexture();
		std::unordered_map<std::string, std::shared_ptr<Texture>>& GetFinalTerrainMasks();
	private:
		void RefreshParticleErosionPreviews(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void RefreshThermalErosionPreviews(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		void RefreshHydrologyGraphOutputs(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks);
		std::unordered_map<std::string, std::shared_ptr<ComputeShader>> m_computeShaders;
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_terrainMasks;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_finalTerrainMasks;
		std::shared_ptr<TerrainGraph> m_graph;
		TerrainGraphAsset m_universalGraphAsset;
		std::vector<VegetationLayerDescriptor> m_vegetationLayers;
		std::uint64_t m_vegetationRevision{ 0 };
		std::string m_proceduralAssetStatus{ "Assets have not been spawned" };
		std::shared_ptr<ShaderManager> m_shaderManager;
		std::shared_ptr<TerrainDebug> m_terrainDebug;

		TerrainPassContext m_terrainPassContext;
		std::unique_ptr<GpuParticleErosion> m_gpuParticleErosion;
		std::unique_ptr<GpuThermalErosion> m_gpuThermalErosion;
		std::unique_ptr<WaterClipmapRenderer> m_waterClipmapRenderer;
		std::unique_ptr<DrainageNetworkGenerator> m_drainageNetworkGenerator;
		bool m_hydrologyGenerated{ false };
		std::string m_hydrologyStatus{ "Hydrology not generated" };
		bool m_particleErosionReady{ false };
		bool m_particleErosionRunning{ false };
		float m_particleStepAccumulator{ 0.0f };
		unsigned int m_particleStepCount{ 0 };
		std::shared_ptr<Texture> m_thermalHardness;
		bool m_thermalErosionReady{ false };
		bool m_thermalErosionRunning{ false };
		bool m_thermalErosionInitialized{ false };
		float m_thermalStepAccumulator{ 0.0f };
		unsigned int m_thermalStepCount{ 0 };
		float m_thermalAmount{ 0.35f };
		float m_thermalTalusAngle{ 32.0f };
		float m_thermalWeathering{ 0.45f };
		int m_size{ 0 };
	};
}
#endif // !SRC_TERRAIN_GPU_TERRAIN_COMPUTE_PIPELINE_H
