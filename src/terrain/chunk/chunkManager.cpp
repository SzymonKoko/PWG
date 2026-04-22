#include "chunkManager.h"
#include "terrain/masks/mask.h"

namespace pwg
{
	ChunkManager::ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial)
		: m_terrainSettings(settings)
	{
		m_worldSize = settings.chunkSize * CHUNKS;

		m_terrainComputePipeline = std::make_shared<TerrainComputePipeline>(m_worldSize, resourceManager->GetShaderManager());
		m_lodManager = std::make_unique<LODManager>(m_terrainSettings.chunkSize);
		m_lodManager->GenerateLODMeshes();

		m_globalMasks = m_terrainComputePipeline->CreateMasks();

		for (int i = 0; i < CHUNKS; i++)
		{
			for (int j = 0; j < CHUNKS; j++)
			{
				auto& chunk = m_chunks[i][j];

				chunk.chunkCoords = glm::ivec2(i, j);
				chunk.material = terrainMaterial;
				chunk.lod = 0;
				chunk.generated = false;
			}
		}

		m_terrainComputePipeline->BuildGraph();

	}

	void ChunkManager::Update(std::shared_ptr<ICamera> camera)
	{
		for (int i = 0; i < CHUNKS; i++)
		{
			for (int j = 0; j < CHUNKS; j++)
			{
				auto& chunk = m_chunks[i][j];

				float worldHalfSize = (m_worldSize - 1) * 0.5f;

				float worldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float worldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				float centerX = worldX + (m_terrainSettings.chunkSize - 1) * 0.5f - worldHalfSize;
				float centerZ = worldZ + (m_terrainSettings.chunkSize - 1) * 0.5f - worldHalfSize;

				glm::vec2 camXZ = glm::vec2(camera->GetCameraPosition().x, camera->GetCameraPosition().z);
				glm::vec2 chunkXZ = glm::vec2(centerX, centerZ);

				float distance = glm::distance(camXZ, chunkXZ);

				m_lodManager->SelectLOD(chunk, distance);
			}
		}

		m_cameraPosition = camera->GetCameraPosition();

		if (m_needRegenerate)
		{
			Regenerate();
		}
	}

	void ChunkManager::Draw()
	{
		float worldHalfSize = (m_worldSize - 1) * 0.5f;

		for (int i = 0; i < CHUNKS; i++)
		{
			for (int j = 0; j < CHUNKS; j++)
			{
				auto& chunk = m_chunks[i][j];
				auto mesh = m_lodManager->GetMesh(chunk.lod);

				glm::vec2 uvOffset = glm::vec2(chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1), chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1)) / glm::vec2(m_worldSize);

				glm::vec2 uvScale = glm::vec2(m_terrainSettings.chunkSize - 1) / glm::vec2(m_worldSize);

				float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				mesh->SetPosition(glm::vec3(chunkWorldX - worldHalfSize, 0, chunkWorldZ - worldHalfSize));

				auto modelMatrix = mesh->GetModelMatrix();

				chunk.material->SetUniformMat4("u_model", modelMatrix);
				chunk.material->SetUniformFloat("u_amplitude", m_amplitude);
				chunk.material->SetUniformTexture("u_Heightmap", m_globalMasks["FinalHeight"]->texture, 0);
				chunk.material->SetUniformTexture("u_Normalmap", m_globalMasks["NormalMask"]->texture, 1);
				chunk.material->SetUniformVec2("u_uvOffset", uvOffset);
				chunk.material->SetUniformVec2("u_uvScale", uvScale);
				chunk.material->SetUniformVec2("u_cameraPosition", m_cameraPosition);
				chunk.material->Apply();

				mesh->Draw();
			}
		}
	}

	void pwg::ChunkManager::Regenerate()
	{
		m_globalMasks["FinalHeight"]->texture->ClearTexture();
		m_globalMasks["NormalMask"]->texture->ClearTexture();

		m_terrainComputePipeline->Execute(m_globalMasks);

		m_needRegenerate = false;
	}

	void ChunkManager::SetNeedRegenerate(bool enabled)
	{
		m_needRegenerate = enabled;
	}

	void ChunkManager::SetAmplitude(float amplitude)
	{
		m_amplitude = amplitude;
	}

	float ChunkManager::GetAmplitude()
	{
		return m_amplitude;
	}

	TerrainChunk& ChunkManager::GetChunk(int x, int y)
	{
		return m_chunks[x][y];
	}

	std::shared_ptr<TerrainComputePipeline> ChunkManager::GetTerrainComputePipeline()
	{
		return m_terrainComputePipeline;
	}
	std::unordered_map<std::string, std::shared_ptr<TerrainMask>> ChunkManager::GetMasks()
	{
		return m_globalMasks;
	}
}


