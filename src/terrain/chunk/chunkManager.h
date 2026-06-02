#ifndef SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
#define SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_

#include "terrainChunk.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/config/terrainSettings.h"
#include <memory>
#include "terrain/lod/LODManager.h"
#include "scene/cameras/icamera.h"
#include "resources/resourceManager.h"
#include <vector>

namespace pwg
{
	class ChunkManager
	{
	public:
		ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<TerrainDebug> debug);
		~ChunkManager() = default;

		void Update(std::shared_ptr<ICamera> camera, float dt);
		void Draw();
		void Regenerate();
		void StepWaterSimulation();
		void StepErosionSimulation();
		void BakeParticleErosion();
		void StartParticleBake();
		void CancelParticleBake();
		void ResetWaterSimulation();
		void ResetErosionSimulation();
		void ConfigureTerrain(int chunkCount, int chunkSize);

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
	
	};
}
#endif // !SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
