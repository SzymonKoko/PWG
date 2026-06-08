#ifndef SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
#define SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_

#include "terrainChunk.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/config/terrainSettings.h"
#include <memory>
#include "terrain/lod/LODManager.h"
#include "scene/cameras/icamera.h"
#include "resources/resourceManager.h"
#include "graphics/texture/texture.h"
#include <glm/glm.hpp>
#include <vector>

namespace pwg
{
	// Snapshot of the live simulation textures so the eroded terrain and the water
	// (rivers / lakes) survive saving and reopening a project.
	struct SimulationStateSnapshot
	{
		int worldSize{ 0 };
		std::vector<float> finalHeight;
		std::vector<float> water;
		std::vector<float> sediment;
		std::vector<float> deposit;
		std::vector<float> flowAccumulation;
		std::vector<float> riverChannels;

		bool HasData() const { return worldSize > 0 && !finalHeight.empty(); }
	};

	class ChunkManager
	{
	public:
		ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<TerrainDebug> debug);
		~ChunkManager() = default;

		void Update(std::shared_ptr<ICamera> camera, float dt);
		void Draw();
		void Regenerate();
		void StepSimulation();
		void StepWaterSimulation();
		void StepErosionSimulation();
		void BeginWaterSettle();
		void CancelWaterSettle();
		void BakeParticleErosion();
		void StartParticleBake();
		void CancelParticleBake();
		void ResetWaterSimulation();
		void ResetErosionSimulation();
		void ConfigureTerrain(int chunkCount, int chunkSize);

		void GenerateWorld();
		void BakeErosionAfterWorldGen();

		SimulationStateSnapshot CaptureSimulationState();
		void RestoreSimulationState(const SimulationStateSnapshot& snapshot);

		void SetNeedRegenerate(bool enabled);
		void SetAmplitude(float amplitude);

		float GetAmplitude();
		int GetChunkCount();
		int GetChunkSize();
		int GetWorldSize();
		bool IsParticleBakeActive() const;
		int GetParticleBakeStep() const;
		int GetParticleBakeTotal() const;
		float SampleHeightAtWorld(float worldX, float worldZ);
		void UpdateHeightCache();
		void MaybeUpdateHeightCache(float dt);

		TerrainChunk& GetChunk(int x, int y);

		std::shared_ptr<TerrainComputePipeline> GetTerrainComputePipeline();
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& GetMasks();
	private:
		std::shared_ptr<TerrainComputePipeline> m_terrainComputePipeline;
		std::shared_ptr<ResourceManager> m_resourceManager;
		std::shared_ptr<Material> m_terrainMaterial;
		std::unique_ptr<LODManager> m_lodManager;
		std::vector<TerrainChunk> m_chunks;
		TerrainSettings m_terrainSettings;
		int m_chunkCount{ 31 };
		bool m_needRegenerate{ true };
		float m_amplitude{ 1.0f };
		int m_worldSize{ 0 };
		glm::vec3 m_cameraPosition{ 0 };
		float m_waterSimulationAccumulator{ 0.0f };
		float m_heightCacheAccumulator{ 0.0f };
		static constexpr float m_heightCacheInterval{ 0.4f };

		bool m_simulationRanThisTick{ false };
		bool m_prevSimulating{ false };
		bool m_waterSettling{ false };
		int m_settleStepsDone{ 0 };
		int m_settleMaxSteps{ 0 };
		bool m_particleBakeActive{ false };
		int m_particleBakeStep{ 0 };
		int m_particleBakeTotal{ 0 };
		std::vector<float> m_heightCache;
		bool m_heightCacheValid{ false };
		std::shared_ptr<TerrainDebug> m_terrainDebug;
		std::shared_ptr<Texture> m_sceneColorCopy;
		int m_sceneColorCopyWidth{ 0 };
		int m_sceneColorCopyHeight{ 0 };

		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_globalMasks;

		glm::vec4 m_frustumPlanes[6]{};
		bool m_frustumValid{ false };

		bool IsChunkVisible(const TerrainChunk& chunk, float worldHalfSize) const;
		void UpdateFrustum(std::shared_ptr<ICamera> camera);
		void SetupSharedTerrainUniforms(bool waterOverlay);
		void DrawTerrainChunks(bool waterOverlay);
	};
}
#endif // !SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
