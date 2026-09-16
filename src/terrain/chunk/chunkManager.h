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
#include "graphics/shader/shader.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace pwg
{
	class Renderer;

	// Snapshot of the live simulation textures so the eroded terrain and the water
	// (rivers / lakes) survive saving and reopening a project.
	struct SimulationStateSnapshot
	{
		int worldSize{ 0 };
		std::vector<float> finalHeight;
		std::vector<float> water;
		std::vector<float> sediment;
		std::vector<float> deposit;
		std::vector<float> erosion;
		std::vector<float> erosionDelta;
		std::vector<float> flowAccumulation;
		std::vector<float> riverChannels;
		std::vector<float> lakes;
		std::vector<float> waterVelocity;
		std::vector<float> riverCarveDelta;
		std::vector<float> hydrologyBaseHeight;
		std::vector<float> thermalBaseHeight;
		std::vector<float> thermalDebris;
		std::vector<float> thermalDebrisMask;
		std::vector<float> thermalErosion;
		std::vector<float> thermalDeposition;
		bool thermalInitialized{ false };

		bool HasData() const { return worldSize > 0 && !finalHeight.empty(); }
	};

	class ChunkManager
	{
	public:
		ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<Material> waterMaterial, std::shared_ptr<TerrainDebug> debug);
		~ChunkManager();

		void Update(std::shared_ptr<ICamera> camera, float dt);
		void Draw();
		void DrawWater(Renderer& renderer);
		void DrawShadowMap(std::shared_ptr<Shader> shader, const glm::mat4& lightSpaceMatrix);
		void Regenerate();
		void CancelParticleErosion();
		void StartParticleErosion();
		void PauseParticleErosion();
		bool StepParticleErosion();
		bool ResetParticleErosion();
		bool StartThermalErosion();
		void PauseThermalErosion();
		bool StepThermalErosion();
		bool ResetThermalErosion();
		void ConfigureTerrain(int chunkCount, int chunkSize);

		void GenerateWorld();
		bool RunUniversalGraph();
		bool RefreshProceduralAssets();
		bool GenerateHydrology();
		void CancelHydrology();
		bool ResetHydrology();
		bool IsHydrologyRunning() const;
		float GetHydrologyProgress() const;
		std::string GetHydrologyStatus() const;
		const std::string& GetProceduralAssetStatus() const;

		SimulationStateSnapshot CaptureSimulationState();
		void RestoreSimulationState(const SimulationStateSnapshot& snapshot);

		void SetNeedRegenerate(bool enabled);
		void SetAmplitude(float amplitude);

		float GetAmplitude();
		int GetChunkCount();
		int GetChunkSize();
		int GetWorldSize();
		std::uint64_t GetGenerationRevision() const;
		bool IsParticleErosionActive() const;
		bool IsParticleErosionReady() const;
		float GetParticleErosionSpeed() const;
		void SetParticleErosionSpeed(float speed);
		bool GetShowErosionParticles() const;
		void SetShowErosionParticles(bool show);
		int GetActiveParticleCount() const;
		bool IsThermalErosionActive() const;
		bool IsThermalErosionReady() const;
		bool IsThermalErosionStable() const;
		float GetThermalErosionSpeed() const;
		void SetThermalErosionSpeed(float speed);
		std::string GetThermalErosionStatus() const;
		float SampleHeightAtWorld(float worldX, float worldZ);
		void UpdateHeightCache();
		bool MaybeUpdateHeightCache(float dt);

		TerrainChunk& GetChunk(int x, int y);

		std::shared_ptr<TerrainComputePipeline> GetTerrainComputePipeline();
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& GetMasks();
	private:
		std::shared_ptr<TerrainComputePipeline> m_terrainComputePipeline;
		std::shared_ptr<ResourceManager> m_resourceManager;
		std::shared_ptr<Material> m_terrainMaterial;
		std::shared_ptr<Material> m_waterMaterial;
		std::unique_ptr<LODManager> m_lodManager;
		std::vector<TerrainChunk> m_chunks;
		TerrainSettings m_terrainSettings;
		int m_chunkCount{ 31 };
		bool m_needRegenerate{ true };
		bool m_proceduralAssetsInitialized{ false };
		float m_amplitude{ 1.0f };
		int m_worldSize{ 0 };
		glm::vec3 m_cameraPosition{ 0 };
		float m_heightCacheAccumulator{ 0.0f };
		static constexpr float m_heightCacheInterval{ 0.25f };

		std::vector<float> m_heightCache;
		bool m_heightCacheValid{ false };
		std::uint64_t m_generationRevision{ 0 };
		std::shared_ptr<TerrainDebug> m_terrainDebug;
		std::shared_ptr<Texture> m_sceneColorCopy;
		unsigned int m_sceneDepthCopy{ 0 };
		int m_sceneColorCopyWidth{ 0 };
		int m_sceneColorCopyHeight{ 0 };
		float m_waterTime{ 0.0f };
		glm::mat4 m_cameraView{ 1.0f };
		glm::mat4 m_cameraProjection{ 1.0f };

		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_globalMasks;

		glm::vec4 m_frustumPlanes[6]{};
		bool m_frustumValid{ false };

		bool IsChunkVisible(const TerrainChunk& chunk, float worldHalfSize) const;
		void UpdateFrustum(std::shared_ptr<ICamera> camera);
		void SetupSharedTerrainUniforms(bool waterOverlay);
		void DrawTerrainChunks(const std::shared_ptr<Material>& material);
	};
}
#endif // !SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
