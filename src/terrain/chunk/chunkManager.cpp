#include "chunkManager.h"
#include "terrain/masks/mask.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>

namespace pwg
{
	ChunkManager::ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<TerrainDebug> debug)
		: m_terrainSettings(settings),
		  m_resourceManager(resourceManager),
		  m_terrainMaterial(terrainMaterial),
		  m_terrainDebug(debug)
	{
		m_worldSize = settings.chunkSize * m_chunkCount;

		m_terrainComputePipeline = std::make_shared<TerrainComputePipeline>(m_worldSize, resourceManager->GetShaderManager(), debug);
		m_lodManager = std::make_unique<LODManager>(m_terrainSettings.chunkSize);
		m_lodManager->GenerateLODMeshes();

		m_globalMasks = m_terrainComputePipeline->CreateMasks();


		m_terrainDebug->RegisterTexture("NONE", DebugMode::DEBUG_NONE, nullptr, false);		
		m_terrainDebug->RegisterTexture("Height", DebugMode::DEBUG_HEIGHT, m_globalMasks["FinalHeight"]->texture, true);		
		m_terrainDebug->RegisterTexture("Normal", DebugMode::DEBUG_NORMAL, nullptr, false);		
		m_terrainDebug->RegisterTexture("Slope", DebugMode::DEBUG_SLOPE, nullptr, false);
		m_terrainDebug->RegisterTexture("Water", DebugMode::DEBUG_WATER, m_globalMasks["WaterMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Sediment", DebugMode::DEBUG_SEDIMENT, m_globalMasks["SedimentMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Deposited Sediment", DebugMode::DEBUG_DEPOSIT, m_globalMasks["DepositMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow - Cardinal", DebugMode::DEBUG_WATER_FLUX, m_globalMasks["WaterFlux"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow - Diagonal", DebugMode::DEBUG_DIAGONAL_FLUX, m_globalMasks["WaterFluxDiagonal"]->texture, true);
		m_terrainDebug->RegisterTexture("Erosion / Deposit Delta", DebugMode::DEBUG_EROSION_DELTA, m_globalMasks["ErosionDelta"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow Accumulation", DebugMode::DEBUG_FLOW_ACCUMULATION, m_globalMasks["FlowAccumulationMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Accumulated River Channels", DebugMode::DEBUG_RIVER_CHANNELS, m_globalMasks["RiverChannelMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Water + Channels", DebugMode::DEBUG_RIVERS_LAKES, m_globalMasks["WaterMask"]->texture, true);

		m_chunks.resize(static_cast<size_t>(m_chunkCount) * static_cast<size_t>(m_chunkCount));
		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];

				chunk.chunkCoords = glm::ivec2(i, j);
				chunk.material = terrainMaterial;
				chunk.lod = 0;
				chunk.generated = false;
			}
		}

		m_terrainComputePipeline->BuildGraph();

	}

	void ChunkManager::Update(std::shared_ptr<ICamera> camera, float dt)
	{
		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];

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

		if (m_particleBakeActive)
		{
			auto& settings = m_terrainComputePipeline->GetHydraulicSettings();
			int stepsPerFrame = std::clamp(settings.bakeStepsPerFrame, 1, 8);
			int uploadInterval = std::max(settings.bakeUploadInterval, 1);

			for (int i = 0; i < stepsPerFrame && m_particleBakeActive; ++i)
			{
				m_particleBakeStep++;
				bool finished = m_particleBakeStep >= m_particleBakeTotal;
				bool uploadPreview = finished || (m_particleBakeStep % uploadInterval == 0);
				m_terrainComputePipeline->StepParticleBake(m_globalMasks, uploadPreview);
				if (uploadPreview)
				{
					UpdateHeightCache();
				}
				if (finished)
				{
					m_particleBakeActive = false;
					UpdateHeightCache();
				}
			}
		}

		auto& hydraulicSettings = m_terrainComputePipeline->GetHydraulicSettings();
		if (hydraulicSettings.autoWaterSimulation)
		{
			float stepsPerSecond = static_cast<float>(std::max(hydraulicSettings.waterStepsPerSecond, 1));
			float stepInterval = 1.0f / stepsPerSecond;
			int substeps = std::clamp(hydraulicSettings.waterSubsteps, 1, 8);
			int maxStepsThisFrame = 2;

			m_waterSimulationAccumulator += std::clamp(dt, 0.0f, 0.1f);
			while (m_waterSimulationAccumulator >= stepInterval && maxStepsThisFrame > 0)
			{
				for (int i = 0; i < substeps; ++i)
				{
					StepWaterSimulation();
				}

				m_waterSimulationAccumulator -= stepInterval;
				maxStepsThisFrame--;
			}

			if (maxStepsThisFrame == 0)
			{
				m_waterSimulationAccumulator = 0.0f;
			}
		}
		else
		{
			m_waterSimulationAccumulator = 0.0f;
		}
	}

	void ChunkManager::Draw()
	{
		float worldHalfSize = (m_worldSize - 1) * 0.5f;

		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];
				auto mesh = m_lodManager->GetMesh(chunk.lod);

				glm::vec2 uvOffset = glm::vec2(chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1), chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1)) / glm::vec2(m_worldSize);

				glm::vec2 uvScale = glm::vec2(m_terrainSettings.chunkSize - 1) / glm::vec2(m_worldSize);

				float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				mesh->SetPosition(glm::vec3(chunkWorldX - worldHalfSize, 0, chunkWorldZ - worldHalfSize));

				auto modelMatrix = mesh->GetModelMatrix();

				chunk.material->SetUniformMat4("u_model", modelMatrix);
				chunk.material->SetUniformFloat("u_amplitude", m_amplitude);
				chunk.material->SetUniformTexture("u_Heightmap", m_globalMasks["FinalHeight"]->texture);
				chunk.material->SetUniformTexture("u_Normalmap", m_globalMasks["NormalMask"]->texture);
				chunk.material->SetUniformTexture("u_Watermap", m_globalMasks["WaterMask"]->texture);
				chunk.material->SetUniformTexture("u_Depositmap", m_globalMasks["DepositMask"]->texture);
				chunk.material->SetUniformTexture("u_FlowAccumulationmap", m_globalMasks["FlowAccumulationMask"]->texture);
				chunk.material->SetUniformTexture("u_RiverChannelmap", m_globalMasks["RiverChannelMask"]->texture);
				chunk.material->SetUniformVec2("u_uvOffset", uvOffset);
				chunk.material->SetUniformVec2("u_uvScale", uvScale);
				chunk.material->SetUniformInt("u_worldSize", m_worldSize);
				chunk.material->SetUniformVec3("u_cameraPosition", m_cameraPosition);
				chunk.material->SetUniformInt("u_renderWaterOverlay", false);

				auto& entries = m_terrainDebug->GetEntries();
				int debugMode = m_terrainDebug->GetDebugMode();
				if (entries.empty())
				{
					continue;
				}
				if (debugMode < 0 || debugMode >= entries.size())
				{
					debugMode = 0;
				}
				auto entry = entries[debugMode];
				
				chunk.material->SetUniformInt("u_debugMode", (int)entry.mode);
				chunk.material->SetUniformInt("u_debugEnabled", m_terrainDebug->GetStatus());

				auto tex = m_globalMasks["FinalHeight"]->texture;
				switch (entry.mode)
				{
				case DebugMode::DEBUG_HEIGHT:
					tex = m_globalMasks["FinalHeight"]->texture;
					break;
				case DebugMode::DEBUG_WATER:
				case DebugMode::DEBUG_RIVERS_LAKES:
					tex = m_globalMasks["WaterMask"]->texture;
					break;
				case DebugMode::DEBUG_SEDIMENT:
					tex = m_globalMasks["SedimentMask"]->texture;
					break;
				case DebugMode::DEBUG_DEPOSIT:
					tex = m_globalMasks["DepositMask"]->texture;
					break;
				case DebugMode::DEBUG_WATER_FLUX:
					tex = m_globalMasks["WaterFlux"]->texture;
					break;
				case DebugMode::DEBUG_DIAGONAL_FLUX:
					tex = m_globalMasks["WaterFluxDiagonal"]->texture;
					break;
				case DebugMode::DEBUG_EROSION_DELTA:
					tex = m_globalMasks["ErosionDelta"]->texture;
					break;
				case DebugMode::DEBUG_FLOW_ACCUMULATION:
					tex = m_globalMasks["FlowAccumulationMask"]->texture;
					break;
				case DebugMode::DEBUG_RIVER_CHANNELS:
					tex = m_globalMasks["RiverChannelMask"]->texture;
					break;
				default:
					if (entry.texture)
					{
						tex = entry.texture;
					}
					break;
				}
				chunk.material->SetUniformTexture("u_debugTexture", tex);
				

				chunk.material->Apply();

				mesh->Draw();
			}
		}

		if (m_terrainDebug->GetStatus() || !m_terrainComputePipeline->GetHydraulicSettings().showWaterSurface)
		{
			return;
		}

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		int viewportWidth = viewport[2];
		int viewportHeight = viewport[3];
		if (viewportWidth <= 0 || viewportHeight <= 0)
		{
			return;
		}

		if (!m_sceneColorCopy || m_sceneColorCopyWidth != viewportWidth || m_sceneColorCopyHeight != viewportHeight)
		{
			m_sceneColorCopy = std::make_shared<Texture>(viewportWidth, viewportHeight, TextureType::ALBEDO);
			m_sceneColorCopyWidth = viewportWidth;
			m_sceneColorCopyHeight = viewportHeight;
		}

		bool useWaterRefraction = m_terrainComputePipeline->GetHydraulicSettings().waterRefractionEnabled;
		if (useWaterRefraction)
		{
			glBindTexture(GL_TEXTURE_2D, m_sceneColorCopy->GetTextureID());
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, viewportWidth, viewportHeight);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);

		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];
				auto mesh = m_lodManager->GetMesh(chunk.lod);

				glm::vec2 uvOffset = glm::vec2(chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1), chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1)) / glm::vec2(m_worldSize);
				glm::vec2 uvScale = glm::vec2(m_terrainSettings.chunkSize - 1) / glm::vec2(m_worldSize);
				float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				mesh->SetPosition(glm::vec3(chunkWorldX - worldHalfSize, 0, chunkWorldZ - worldHalfSize));

				chunk.material->SetUniformMat4("u_model", mesh->GetModelMatrix());
				chunk.material->SetUniformVec2("u_uvOffset", uvOffset);
				chunk.material->SetUniformVec2("u_uvScale", uvScale);
				chunk.material->SetUniformInt("u_renderWaterOverlay", true);
				chunk.material->SetUniformInt("u_useWaterRefraction", useWaterRefraction);
				if (useWaterRefraction)
				{
					chunk.material->SetUniformTexture("u_SceneColor", m_sceneColorCopy);
				}
				chunk.material->SetUniformVec2("u_viewportSize", glm::vec2(viewportWidth, viewportHeight));
				chunk.material->Apply();
				mesh->Draw();
			}
		}

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	void pwg::ChunkManager::Regenerate()
	{
		m_particleBakeActive = false;
		m_globalMasks["FinalHeight"]->texture->ClearTexture();
		m_globalMasks["BaseHeight"]->texture->ClearTexture();
		m_globalMasks["NormalMask"]->texture->ClearTexture();
		m_globalMasks["WaterMask"]->texture->ClearTexture();
		m_globalMasks["SedimentMask"]->texture->ClearTexture();
		m_globalMasks["DepositMask"]->texture->ClearTexture();
		m_globalMasks["WaterFlux"]->texture->ClearTexture();
		m_globalMasks["WaterFluxDiagonal"]->texture->ClearTexture();
		m_globalMasks["ErosionDelta"]->texture->ClearTexture();
		m_globalMasks["FlowAccumulationMask"]->texture->ClearTexture();
		m_globalMasks["RiverChannelMask"]->texture->ClearTexture();

		m_terrainComputePipeline->RegenerateTerrain(m_globalMasks);
		UpdateHeightCache();

		m_needRegenerate = false;
	}

	void ChunkManager::StepErosionSimulation()
	{
		m_particleBakeActive = false;
		m_terrainComputePipeline->StepErosionSimulation(m_globalMasks);
		UpdateHeightCache();
	}

	void ChunkManager::BakeParticleErosion()
	{
		m_terrainComputePipeline->BakeParticleErosion(m_globalMasks);
		UpdateHeightCache();
	}

	void ChunkManager::StartParticleBake()
	{
		auto& settings = m_terrainComputePipeline->GetHydraulicSettings();
		m_particleBakeTotal = std::max(settings.bakeParticleSteps, 1);
		m_particleBakeStep = 0;
		m_particleBakeActive = true;
	}

	void ChunkManager::CancelParticleBake()
	{
		m_particleBakeActive = false;
		m_particleBakeStep = 0;
		m_particleBakeTotal = 0;
		UpdateHeightCache();
	}

	void ChunkManager::StepWaterSimulation()
	{
		m_terrainComputePipeline->StepWaterSimulation(m_globalMasks);
	}

	void ChunkManager::ResetWaterSimulation()
	{
		m_particleBakeActive = false;
		m_terrainComputePipeline->ResetWaterSimulation(m_globalMasks);
	}

	void ChunkManager::ResetErosionSimulation()
	{
		m_particleBakeActive = false;
		m_globalMasks["FinalHeight"]->texture->ClearTexture();
		m_globalMasks["NormalMask"]->texture->ClearTexture();
		m_globalMasks["WaterMask"]->texture->ClearTexture();
		m_globalMasks["SedimentMask"]->texture->ClearTexture();
		m_globalMasks["DepositMask"]->texture->ClearTexture();
		m_globalMasks["ErosionDelta"]->texture->ClearTexture();
		m_terrainComputePipeline->ResetErosionSimulation(m_globalMasks);
		UpdateHeightCache();
		m_needRegenerate = false;
	}

	void ChunkManager::ConfigureTerrain(int chunkCount, int chunkSize)
	{
		m_particleBakeActive = false;
		chunkCount = std::clamp(chunkCount, 3, 31);
		chunkSize = std::clamp(chunkSize, 33, 257);
		if (chunkCount == m_chunkCount && chunkSize == m_terrainSettings.chunkSize)
		{
			return;
		}

		m_chunkCount = chunkCount;
		m_terrainSettings.chunkSize = chunkSize;
		m_worldSize = m_terrainSettings.chunkSize * m_chunkCount;
		m_terrainComputePipeline = std::make_shared<TerrainComputePipeline>(m_worldSize, m_resourceManager->GetShaderManager(), m_terrainDebug);
		m_lodManager = std::make_unique<LODManager>(m_terrainSettings.chunkSize);
		m_lodManager->GenerateLODMeshes();
		m_globalMasks = m_terrainComputePipeline->CreateMasks();

		m_terrainDebug->Clear();
		m_terrainDebug->RegisterTexture("NONE", DebugMode::DEBUG_NONE, nullptr, false);
		m_terrainDebug->RegisterTexture("Height", DebugMode::DEBUG_HEIGHT, m_globalMasks["FinalHeight"]->texture, true);
		m_terrainDebug->RegisterTexture("Normal", DebugMode::DEBUG_NORMAL, nullptr, false);
		m_terrainDebug->RegisterTexture("Slope", DebugMode::DEBUG_SLOPE, nullptr, false);
		m_terrainDebug->RegisterTexture("Water", DebugMode::DEBUG_WATER, m_globalMasks["WaterMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Sediment", DebugMode::DEBUG_SEDIMENT, m_globalMasks["SedimentMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Deposited Sediment", DebugMode::DEBUG_DEPOSIT, m_globalMasks["DepositMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow - Cardinal", DebugMode::DEBUG_WATER_FLUX, m_globalMasks["WaterFlux"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow - Diagonal", DebugMode::DEBUG_DIAGONAL_FLUX, m_globalMasks["WaterFluxDiagonal"]->texture, true);
		m_terrainDebug->RegisterTexture("Erosion / Deposit Delta", DebugMode::DEBUG_EROSION_DELTA, m_globalMasks["ErosionDelta"]->texture, true);
		m_terrainDebug->RegisterTexture("Flow Accumulation", DebugMode::DEBUG_FLOW_ACCUMULATION, m_globalMasks["FlowAccumulationMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Accumulated River Channels", DebugMode::DEBUG_RIVER_CHANNELS, m_globalMasks["RiverChannelMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Water + Channels", DebugMode::DEBUG_RIVERS_LAKES, m_globalMasks["WaterMask"]->texture, true);

		m_chunks.clear();
		m_chunks.resize(static_cast<size_t>(m_chunkCount) * static_cast<size_t>(m_chunkCount));
		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];
				chunk.chunkCoords = glm::ivec2(i, j);
				chunk.material = m_terrainMaterial;
				chunk.lod = 0;
				chunk.generated = false;
			}
		}

		m_terrainComputePipeline->BuildGraph();
		m_heightCache.clear();
		m_heightCacheValid = false;
		m_needRegenerate = true;
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

	int ChunkManager::GetChunkCount()
	{
		return m_chunkCount;
	}

	int ChunkManager::GetChunkSize()
	{
		return m_terrainSettings.chunkSize;
	}

	int ChunkManager::GetWorldSize()
	{
		return m_worldSize;
	}

	bool ChunkManager::IsParticleBakeActive() const
	{
		return m_particleBakeActive;
	}

	int ChunkManager::GetParticleBakeStep() const
	{
		return m_particleBakeStep;
	}

	int ChunkManager::GetParticleBakeTotal() const
	{
		return m_particleBakeTotal;
	}

	float ChunkManager::SampleHeightAtWorld(float worldX, float worldZ)
	{
		if (!m_heightCacheValid || m_heightCache.empty() || m_worldSize <= 1)
		{
			return 0.0f;
		}

		float worldHalfSize = (m_worldSize - 1) * 0.5f;
		float textureX = std::clamp(worldX + worldHalfSize, 0.0f, static_cast<float>(m_worldSize - 1));
		float textureZ = std::clamp(worldZ + worldHalfSize, 0.0f, static_cast<float>(m_worldSize - 1));

		int x0 = static_cast<int>(std::floor(textureX));
		int z0 = static_cast<int>(std::floor(textureZ));
		int x1 = std::min(x0 + 1, m_worldSize - 1);
		int z1 = std::min(z0 + 1, m_worldSize - 1);
		float tx = textureX - static_cast<float>(x0);
		float tz = textureZ - static_cast<float>(z0);

		auto heightAt = [&](int x, int z)
		{
			size_t index = static_cast<size_t>(z) * static_cast<size_t>(m_worldSize) + static_cast<size_t>(x);
			return m_heightCache[index] * m_amplitude;
		};

		float h00 = heightAt(x0, z0);
		float h10 = heightAt(x1, z0);
		float h01 = heightAt(x0, z1);
		float h11 = heightAt(x1, z1);
		float hx0 = glm::mix(h00, h10, tx);
		float hx1 = glm::mix(h01, h11, tx);
		return glm::mix(hx0, hx1, tz);
	}

	void ChunkManager::UpdateHeightCache()
	{
		auto heightMask = m_globalMasks.find("FinalHeight");
		if (heightMask == m_globalMasks.end() || !heightMask->second || !heightMask->second->texture)
		{
			m_heightCacheValid = false;
			return;
		}

		m_heightCache = heightMask->second->texture->ReadFloatData();
		m_heightCacheValid = m_heightCache.size() >= static_cast<size_t>(m_worldSize) * static_cast<size_t>(m_worldSize);
	}

	TerrainChunk& ChunkManager::GetChunk(int x, int y)
	{
		return m_chunks[static_cast<size_t>(y) * m_chunkCount + x];
	}

	std::shared_ptr<TerrainComputePipeline> ChunkManager::GetTerrainComputePipeline()
	{
		return m_terrainComputePipeline;
	}
	std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& ChunkManager::GetMasks()
	{
		return m_globalMasks;
	}
}


