#ifndef SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
#define SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_

#include "terrainChunk.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/config/terrainSettings.h"
#include <memory>
#include "terrain/lod/LODManager.h"
#include "scene/cameras/icamera.h"
#include "resources/resourceManager.h"

#define CHUNKS 31

namespace pwg
{
	class ChunkManager
	{
	public:
		ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<TerrainDebug> debug);
		~ChunkManager() = default;

		void Update(std::shared_ptr<ICamera> camera);
		void Draw();
		void Regenerate();

		void SetNeedRegenerate(bool enabled);
		void SetAmplitude(float amplitude);

		float GetAmplitude();

		TerrainChunk& GetChunk(int x, int y);

		std::shared_ptr<TerrainComputePipeline> GetTerrainComputePipeline();
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> GetMasks();
	private:
		std::shared_ptr<TerrainComputePipeline> m_terrainComputePipeline;
		std::unique_ptr<LODManager> m_lodManager;
		TerrainChunk m_chunks[CHUNKS][CHUNKS];
		TerrainSettings m_terrainSettings;
		bool m_needRegenerate{ true };
		float m_amplitude{ 0 };
		int m_worldSize{ 0 };
		glm::vec3 m_cameraPosition{ 0 };
		std::shared_ptr<TerrainDebug> m_terrainDebug;

		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> m_globalMasks;
	
	};
}
#endif // !SRC_TERRAIN_CHUNK_CHUNK_MANAGER_H_
