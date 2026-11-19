#include "chunkManager.h"
#include "terrain/masks/mask.h"
#include "graphics/renderer/renderer.h"
#include "debug/debugView.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace pwg
{
	ChunkManager::ChunkManager(std::shared_ptr<ResourceManager> resourceManager, TerrainSettings& settings, std::shared_ptr<Material> terrainMaterial, std::shared_ptr<Material> waterMaterial, std::shared_ptr<TerrainDebug> debug)
		: m_terrainSettings(settings),
		  m_resourceManager(resourceManager),
		  m_terrainMaterial(terrainMaterial),
		  m_waterMaterial(waterMaterial),
		  m_terrainDebug(debug)
	{
		m_worldSize = settings.chunkSize * m_chunkCount;

		m_terrainComputePipeline = std::make_shared<TerrainComputePipeline>(m_worldSize, resourceManager->GetShaderManager(), debug);
		m_lodManager = std::make_unique<LODManager>(m_terrainSettings.chunkSize);
		m_lodManager->GenerateLODMeshes();

		m_globalMasks = m_terrainComputePipeline->CreateMasks();
		m_proceduralAssetsInitialized = false;


		m_terrainDebug->RegisterTexture("NONE", DebugMode::DEBUG_NONE, nullptr, false);		
		m_terrainDebug->RegisterTexture("Height", DebugMode::DEBUG_HEIGHT, m_globalMasks["FinalHeight"]->texture, true);		
		m_terrainDebug->RegisterTexture("Normal", DebugMode::DEBUG_NORMAL, nullptr, false);		
		m_terrainDebug->RegisterTexture("Slope", DebugMode::DEBUG_SLOPE, nullptr, false);
		m_terrainDebug->RegisterTexture("Water", DebugMode::DEBUG_WATER, m_globalMasks["WaterMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Sediment", DebugMode::DEBUG_SEDIMENT, m_globalMasks["SedimentMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Deposited Sediment", DebugMode::DEBUG_DEPOSIT, m_globalMasks["DepositMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Generated Flow", DebugMode::DEBUG_WATER_FLUX, m_globalMasks["FlowAccumulationMask"]->texture, true);
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

	ChunkManager::~ChunkManager()
	{
		ResetHeightCacheReadbacks();
		for (auto& slot : m_heightReadbacks)
		{
			if (slot.buffer != 0) glDeleteBuffers(1, &slot.buffer);
		}
		if (m_sceneDepthCopy != 0) glDeleteTextures(1, &m_sceneDepthCopy);
	}

	void ChunkManager::Update(std::shared_ptr<ICamera> camera, float dt)
	{
		m_waterTime += std::clamp(dt, 0.0f, 0.1f);
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
		m_cameraView = camera->GetViewMatrix();
		m_cameraProjection = camera->GetProjectionMatrix();
		if (!m_cullingFrozen)
		{
			UpdateFrustum(camera);
		}

		if (m_needRegenerate)
		{
			Regenerate();
		}
		if (m_terrainComputePipeline->UpdateParticleErosion(m_globalMasks, dt))
		{
			if (MaybeUpdateHeightCache(dt)) ++m_generationRevision;
		}
		const bool thermalWasRunning = m_terrainComputePipeline->IsThermalErosionActive();
		if (m_terrainComputePipeline->UpdateThermalErosion(m_globalMasks, dt))
		{
			if (MaybeUpdateHeightCache(dt)) ++m_generationRevision;
		}
		if (thermalWasRunning && !m_terrainComputePipeline->IsThermalErosionActive())
		{
			m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
			UpdateHeightCache();
			++m_generationRevision;
		}
		if (m_terrainComputePipeline->UpdateHydrology(m_globalMasks))
		{
			UpdateHeightCache();
			++m_generationRevision;
		}

	}

	void ChunkManager::SetLodEnabled(bool enabled) { m_lodManager->SetEnabled(enabled); }
	bool ChunkManager::IsLodEnabled() const { return m_lodManager->IsEnabled(); }
	bool ChunkManager::SetLodScale(float scale) { return m_lodManager->SetScale(scale); }
	float ChunkManager::GetLodScale() const { return m_lodManager->GetScale(); }
	bool ChunkManager::SetLodDistances(const LODManager::Thresholds& distances) { return m_lodManager->SetDistances(distances); }
	const LODManager::Thresholds& ChunkManager::GetLodDistances() const { return m_lodManager->GetDistances(); }
	void ChunkManager::ResetLodDistances() { m_lodManager->ResetDistances(); }

	void ChunkManager::UpdateFrustum(std::shared_ptr<ICamera> camera)
	{
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 proj = camera->GetProjectionMatrix();
		glm::mat4 viewProj = proj * view;

		m_frustumPlanes[0] = glm::vec4(viewProj[0][3] + viewProj[0][0], viewProj[1][3] + viewProj[1][0], viewProj[2][3] + viewProj[2][0], viewProj[3][3] + viewProj[3][0]);
		m_frustumPlanes[1] = glm::vec4(viewProj[0][3] - viewProj[0][0], viewProj[1][3] - viewProj[1][0], viewProj[2][3] - viewProj[2][0], viewProj[3][3] - viewProj[3][0]);
		m_frustumPlanes[2] = glm::vec4(viewProj[0][3] + viewProj[0][1], viewProj[1][3] + viewProj[1][1], viewProj[2][3] + viewProj[2][1], viewProj[3][3] + viewProj[3][1]);
		m_frustumPlanes[3] = glm::vec4(viewProj[0][3] - viewProj[0][1], viewProj[1][3] - viewProj[1][1], viewProj[2][3] - viewProj[2][1], viewProj[3][3] - viewProj[3][1]);
		m_frustumPlanes[4] = glm::vec4(viewProj[0][3] + viewProj[0][2], viewProj[1][3] + viewProj[1][2], viewProj[2][3] + viewProj[2][2], viewProj[3][3] + viewProj[3][2]);
		m_frustumPlanes[5] = glm::vec4(viewProj[0][3] - viewProj[0][2], viewProj[1][3] - viewProj[1][2], viewProj[2][3] - viewProj[2][2], viewProj[3][3] - viewProj[3][2]);

		for (int i = 0; i < 6; ++i)
		{
			float length = glm::length(glm::vec3(m_frustumPlanes[i]));
			if (length > 0.0f)
			{
				m_frustumPlanes[i] /= length;
			}
		}
		m_frustumValid = true;
	}

	bool ChunkManager::IsChunkVisible(const TerrainChunk& chunk, float worldHalfSize) const
	{
		if (!m_cullingEnabled || !m_frustumValid)
		{
			return true;
		}

		// The vertex shader places geometry at the raw heightmap value (pos.y = h)
		// in world units, which can range up to several thousand. Using m_amplitude
		// here is wrong: it produced a paper-thin AABB so tall chunks near the camera
		// were culled while flat distant ones survived. Cull on the XZ footprint with
		// a very generous vertical extent so the test is effectively horizontal.
		const float edgeMargin = (m_terrainSettings.chunkSize - 1) * 0.5f;
		float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
		float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);
		glm::vec3 minBounds(
			chunkWorldX - worldHalfSize - edgeMargin,
			-100000.0f,
			chunkWorldZ - worldHalfSize - edgeMargin);
		glm::vec3 maxBounds(
			chunkWorldX - worldHalfSize + (m_terrainSettings.chunkSize - 1) + edgeMargin,
			100000.0f,
			chunkWorldZ - worldHalfSize + (m_terrainSettings.chunkSize - 1) + edgeMargin);

		for (int i = 0; i < 6; ++i)
		{
			glm::vec3 positive(
				m_frustumPlanes[i].x >= 0.0f ? maxBounds.x : minBounds.x,
				m_frustumPlanes[i].y >= 0.0f ? maxBounds.y : minBounds.y,
				m_frustumPlanes[i].z >= 0.0f ? maxBounds.z : minBounds.z);
			if (glm::dot(glm::vec3(m_frustumPlanes[i]), positive) + m_frustumPlanes[i].w < 0.0f)
			{
				return false;
			}
		}
		return true;
	}

	void ChunkManager::SetupSharedTerrainUniforms(bool waterOverlay)
	{
		auto& entries = m_terrainDebug->GetEntries();
		if (entries.empty())
		{
			return;
		}

		int debugMode = m_terrainDebug->GetDebugMode();
		if (debugMode < 0 || debugMode >= static_cast<int>(entries.size()))
		{
			debugMode = 0;
		}
		const auto& entry = entries[debugMode];

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
		case DebugMode::DEBUG_DIAGONAL_FLUX:
			tex = m_globalMasks["FlowAccumulationMask"]->texture;
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

		m_terrainMaterial->SetUniformFloat("u_amplitude", m_amplitude);
		m_terrainMaterial->SetUniformTexture("u_Heightmap", m_globalMasks["FinalHeight"]->texture);
		m_terrainMaterial->SetUniformTexture("u_Normalmap", m_globalMasks["NormalMask"]->texture);
		m_terrainMaterial->SetUniformTexture("u_Watermap", m_globalMasks["WaterMask"]->texture);
		m_terrainMaterial->SetUniformTexture("u_Depositmap", m_globalMasks["DepositMask"]->texture);
		m_terrainMaterial->SetUniformTexture("u_FlowAccumulationmap", m_globalMasks["FlowAccumulationMask"]->texture);
		m_terrainMaterial->SetUniformTexture("u_RiverChannelmap", m_globalMasks["RiverChannelMask"]->texture);
		m_terrainMaterial->SetUniformInt("u_worldSize", m_worldSize);
		m_terrainMaterial->SetUniformVec3("u_cameraPosition", m_cameraPosition);
		m_terrainMaterial->SetUniformInt("u_renderWaterOverlay", waterOverlay ? 1 : 0);
		m_terrainMaterial->SetUniformInt("u_debugMode", static_cast<int>(entry.mode));
		m_terrainMaterial->SetUniformInt("u_debugEnabled", m_terrainDebug->GetStatus());
		m_terrainMaterial->SetUniformTexture("u_debugTexture", tex);
		// Debug View overlays (D-05): x = flags (bit 0 LOD tint, bit 1 chunk borders; 0 = off), y = LOD of the
		// chunk being drawn (set per chunk in DrawTerrainChunks while the tint is on), z = chunk edge length.
		m_terrainMaterial->SetUniformVec3("u_chunkDebug", glm::vec3(
			static_cast<float>((m_lodTint ? 1 : 0) | (m_chunkBorders ? 2 : 0)),
			0.0f,
			static_cast<float>(m_terrainSettings.chunkSize - 1)));
		m_terrainMaterial->Apply();
	}

	void ChunkManager::DrawTerrainChunks(const std::shared_ptr<Material>& material)
	{
		float worldHalfSize = (m_worldSize - 1) * 0.5f;
		auto shader = material ? material->GetShader() : nullptr;
		if (!shader)
		{
			return;
		}

		static_assert(MAX_LOD == static_cast<int>(debug::TerrainLodCount), "Debug View counts TerrainLodCount LOD levels");
		const bool tintChunks = m_lodTint;
		const float chunkEdge = static_cast<float>(m_terrainSettings.chunkSize - 1);
		const float overlayFlags = static_cast<float>((m_lodTint ? 1 : 0) | (m_chunkBorders ? 2 : 0));
		PWG_RENDER_COUNT(SetChunkCount(static_cast<uint32_t>(m_chunkCount * m_chunkCount)));
		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];
				if (!IsChunkVisible(chunk, worldHalfSize))
				{
					continue;
				}
				PWG_RENDER_COUNT(AddVisibleChunk(chunk.lod));

				auto mesh = m_lodManager->GetMesh(chunk.lod);
				glm::vec2 uvOffset = (glm::vec2(chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1), chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1)) + glm::vec2(0.5f)) / glm::vec2(m_worldSize);
				glm::vec2 uvScale = glm::vec2(m_terrainSettings.chunkSize - 1) / glm::vec2(m_worldSize);
				float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				mesh->SetPosition(glm::vec3(chunkWorldX - worldHalfSize, 0, chunkWorldZ - worldHalfSize));
				auto modelMatrix = mesh->GetModelMatrix();

				shader->SetUniformMat4("u_model", modelMatrix);
				shader->SetUniformVec2("u_uvOffset", uvOffset);
				shader->SetUniformVec2("u_uvScale", uvScale);
				if (tintChunks)
				{
					shader->SetUniformVec3("u_chunkDebug", glm::vec3(overlayFlags, static_cast<float>(chunk.lod), chunkEdge));
				}
				mesh->Draw();
			}
		}
	}

	void ChunkManager::DrawShadowMap(std::shared_ptr<Shader> shader, const glm::mat4& lightSpaceMatrix)
	{
		if (!shader)
		{
			return;
		}

		auto heightMask = m_globalMasks.find("FinalHeight");
		if (heightMask == m_globalMasks.end() || !heightMask->second || !heightMask->second->texture)
		{
			return;
		}

		float worldHalfSize = (m_worldSize - 1) * 0.5f;
		shader->ActivateShader();
		shader->SetUniformMat4("u_lightSpaceMatrix", lightSpaceMatrix);
		shader->SetUniformInt("u_Heightmap", 0);
		heightMask->second->texture->Bind(0);

		for (int i = 0; i < m_chunkCount; i++)
		{
			for (int j = 0; j < m_chunkCount; j++)
			{
				auto& chunk = m_chunks[static_cast<size_t>(j) * m_chunkCount + i];
				auto mesh = m_lodManager->GetMesh(chunk.lod);
				glm::vec2 uvOffset = (glm::vec2(chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1), chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1)) + glm::vec2(0.5f)) / glm::vec2(m_worldSize);
				glm::vec2 uvScale = glm::vec2(m_terrainSettings.chunkSize - 1) / glm::vec2(m_worldSize);
				float chunkWorldX = chunk.chunkCoords.x * (m_terrainSettings.chunkSize - 1);
				float chunkWorldZ = chunk.chunkCoords.y * (m_terrainSettings.chunkSize - 1);

				mesh->SetPosition(glm::vec3(chunkWorldX - worldHalfSize, 0, chunkWorldZ - worldHalfSize));
				shader->SetUniformMat4("u_model", mesh->GetModelMatrix());
				shader->SetUniformVec2("u_uvOffset", uvOffset);
				shader->SetUniformVec2("u_uvScale", uvScale);
				mesh->Draw();
				PWG_RENDER_COUNT(AddShadowChunk());
			}
		}
		heightMask->second->texture->Unbind();
	}

	void ChunkManager::Draw()
	{
		if (m_terrainDebug->GetEntries().empty())
		{
			return;
		}

		SetupSharedTerrainUniforms(false);
		DrawTerrainChunks(m_terrainMaterial);
		m_terrainComputePipeline->DrawErosionParticles(m_cameraView, m_cameraProjection);
	}

	void ChunkManager::DrawWater(Renderer& renderer)
	{
		if (m_terrainDebug->GetStatus() || !m_terrainComputePipeline->GetHydraulicSettings().showWaterSurface)
		{
			return;
		}
		if (!m_waterMaterial || !m_waterMaterial->GetShader()) return;

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
			m_sceneColorCopy = std::make_shared<Texture>(viewportWidth, viewportHeight, TextureType::HDR_COLOR);
			m_sceneColorCopyWidth = viewportWidth;
			m_sceneColorCopyHeight = viewportHeight;
			if (m_sceneDepthCopy != 0) glDeleteTextures(1, &m_sceneDepthCopy);
			glGenTextures(1, &m_sceneDepthCopy);
			glBindTexture(GL_TEXTURE_2D, m_sceneDepthCopy);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, viewportWidth, viewportHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glBindTexture(GL_TEXTURE_2D, m_sceneColorCopy->GetTextureID());
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, viewportWidth, viewportHeight);
		glBindTexture(GL_TEXTURE_2D, m_sceneDepthCopy);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, viewportWidth, viewportHeight);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDepthMask(GL_FALSE);
		renderer.PrepareMaterial(m_waterMaterial);
		m_waterMaterial->SetUniformTexture("u_Heightmap", m_globalMasks["FinalHeight"]->texture);
		m_waterMaterial->SetUniformTexture("u_Watermap", m_globalMasks["WaterMask"]->texture);
		m_waterMaterial->SetUniformTexture("u_FlowAccumulationmap", m_globalMasks["FlowAccumulationMask"]->texture);
		m_waterMaterial->SetUniformTexture("u_RiverChannelmap", m_globalMasks["RiverChannelMask"]->texture);
		m_waterMaterial->SetUniformTexture("u_WaterVelocityMap", m_globalMasks["WaterVelocityMap"]->texture);
		m_waterMaterial->SetUniformTexture("u_SceneColor", m_sceneColorCopy);
		m_waterMaterial->SetUniformVec2("u_viewportSize", glm::vec2(viewportWidth, viewportHeight));
		m_waterMaterial->SetUniformFloat("u_time", m_waterTime);
		m_waterMaterial->SetUniformFloat("u_minVisibleWaterDepth", m_terrainComputePipeline->GetHydraulicSettings().minVisibleWaterDepth);
		m_waterMaterial->SetUniformInt("u_useWaterRefraction", m_terrainComputePipeline->GetHydraulicSettings().waterRefractionEnabled ? 1 : 0);
		m_waterMaterial->Apply();
		auto waterShader = m_waterMaterial->GetShader();
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, m_sceneDepthCopy);
		waterShader->SetUniformInt("u_SceneDepth", 15);
		m_terrainComputePipeline->DrawWaterClipmap(waterShader, m_cameraPosition, m_globalMasks);

		glDepthMask(GL_TRUE);
	}

	void pwg::ChunkManager::Regenerate()
	{
		CancelHydrology();
		m_terrainComputePipeline->CancelParticleErosion();
		m_terrainComputePipeline->PauseThermalErosion();
		m_terrainComputePipeline->RegenerateTerrain(m_globalMasks);
		if (!m_terrainComputePipeline->IsParticleErosionActive() && !m_terrainComputePipeline->IsThermalErosionActive()) UpdateHeightCache();

		m_needRegenerate = false;
	}

	SimulationStateSnapshot ChunkManager::CaptureSimulationState()
	{
		SimulationStateSnapshot snapshot;
		snapshot.worldSize = m_worldSize;

		auto read = [&](const char* name) -> std::vector<float>
		{
			auto it = m_globalMasks.find(name);
			if (it == m_globalMasks.end() || !it->second || !it->second->texture)
			{
				return {};
			}
			return it->second->texture->ReadFloatData();
		};

		snapshot.finalHeight = read("FinalHeight");
		snapshot.water = read("WaterMask");
		snapshot.sediment = read("SedimentMask");
		snapshot.deposit = read("DepositMask");
		snapshot.erosion = read("ErosionMask");
		snapshot.erosionDelta = read("ErosionDelta");
		snapshot.flowAccumulation = read("FlowAccumulationMask");
		snapshot.riverChannels = read("RiverChannelMask");
		snapshot.lakes = read("LakeMask");
		snapshot.waterVelocity = read("WaterVelocityMap");
		snapshot.riverCarveDelta = read("RiverCarveDelta");
		snapshot.hydrologyBaseHeight = read("HydrologyBaseHeight");
		snapshot.thermalInitialized = m_terrainComputePipeline->HasThermalErosionState();
		if (snapshot.thermalInitialized)
		{
			snapshot.thermalBaseHeight = read("ThermalBaseHeight");
			snapshot.thermalDebris = read("ThermalDebris");
			snapshot.thermalDebrisMask = read("ThermalDebrisMask");
			snapshot.thermalErosion = read("ThermalErosionMask");
			snapshot.thermalDeposition = read("ThermalDepositionMask");
		}
		return snapshot;
	}

	void ChunkManager::RestoreSimulationState(const SimulationStateSnapshot& snapshot)
	{
		if (!snapshot.HasData() || snapshot.worldSize != m_worldSize)
		{
			return;
		}
		m_terrainComputePipeline->CancelParticleErosion();
		m_terrainComputePipeline->PauseThermalErosion();

		size_t expected = static_cast<size_t>(m_worldSize) * static_cast<size_t>(m_worldSize);
		auto write = [&](const char* name, const std::vector<float>& data)
		{
			if (data.size() < expected)
			{
				return;
			}
			auto it = m_globalMasks.find(name);
			if (it == m_globalMasks.end() || !it->second || !it->second->texture)
			{
				return;
			}
			it->second->texture->UpdateFloatData(data, m_worldSize, m_worldSize);
		};

		write("FinalHeight", snapshot.finalHeight);
		write("WaterMask", snapshot.water);
		write("SedimentMask", snapshot.sediment);
		write("DepositMask", snapshot.deposit);
		write("ErosionMask", snapshot.erosion);
		write("ErosionDelta", snapshot.erosionDelta);
		write("FlowAccumulationMask", snapshot.flowAccumulation);
		write("RiverChannelMask", snapshot.riverChannels);
		write("LakeMask", snapshot.lakes);
		if (snapshot.waterVelocity.size() >= expected * 2)
			m_globalMasks["WaterVelocityMap"]->texture->UpdateFloatData(snapshot.waterVelocity, m_worldSize, m_worldSize);
		write("RiverCarveDelta", snapshot.riverCarveDelta);
		write("HydrologyBaseHeight", snapshot.hydrologyBaseHeight);
		if (snapshot.thermalInitialized)
		{
			write("ThermalBaseHeight", snapshot.thermalBaseHeight);
			write("ThermalDebris", snapshot.thermalDebris);
			write("ThermalDebrisMask", snapshot.thermalDebrisMask);
			write("ThermalErosionMask", snapshot.thermalErosion);
			write("ThermalDepositionMask", snapshot.thermalDeposition);
		}
		m_terrainComputePipeline->MarkHydrologyRestored(m_globalMasks, snapshot.hydrologyBaseHeight.size() >= expected);
		m_terrainComputePipeline->MarkThermalErosionRestored(
			m_globalMasks, snapshot.thermalInitialized && snapshot.thermalBaseHeight.size() >= expected);

		m_globalMasks["ErodedHeight"] = m_globalMasks["FinalHeight"];

		// Recompute normals / slope from the restored heightmap so shading matches.
		m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
		m_terrainComputePipeline->InvalidateWaterRenderCache();
		UpdateHeightCache();
		++m_generationRevision;
	}

	void ChunkManager::CancelParticleErosion()
	{
		m_terrainComputePipeline->CancelParticleErosion();
	}

	void ChunkManager::StartParticleErosion()
	{
		if (m_terrainComputePipeline->HasThermalErosionState())
			m_terrainComputePipeline->ResetThermalErosionState(m_globalMasks);
		if (m_terrainComputePipeline->ResetHydrology(m_globalMasks)) UpdateHeightCache();
		m_terrainComputePipeline->StartParticleErosion();
	}

	bool ChunkManager::GenerateHydrology()
	{
		return m_terrainComputePipeline->GenerateHydrology(m_globalMasks);
	}

	void ChunkManager::CancelHydrology() { m_terrainComputePipeline->CancelHydrology(); }

	bool ChunkManager::ResetHydrology()
	{
		if (!m_terrainComputePipeline->ResetHydrology(m_globalMasks)) return false;
		UpdateHeightCache();
		++m_generationRevision;
		return true;
	}

	bool ChunkManager::IsHydrologyRunning() const { return m_terrainComputePipeline->IsHydrologyRunning(); }
	float ChunkManager::GetHydrologyProgress() const { return m_terrainComputePipeline->GetHydrologyProgress(); }
	std::string ChunkManager::GetHydrologyStatus() const { return m_terrainComputePipeline->GetHydrologyStatus(); }

	void ChunkManager::PauseParticleErosion()
	{
		m_terrainComputePipeline->PauseParticleErosion();
		m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
		UpdateHeightCache();
		++m_generationRevision;
	}

	bool ChunkManager::StepParticleErosion()
	{
		if (m_terrainComputePipeline->HasThermalErosionState())
			m_terrainComputePipeline->ResetThermalErosionState(m_globalMasks);
		if (!m_terrainComputePipeline->StepParticleErosion(m_globalMasks)) return false;
		m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
		UpdateHeightCache();
		++m_generationRevision;
		return true;
	}

	bool ChunkManager::ResetParticleErosion()
	{
		if (m_terrainComputePipeline->HasThermalErosionState())
			m_terrainComputePipeline->ResetThermalErosionState(m_globalMasks);
		const bool reset = m_terrainComputePipeline->ResetParticleErosionState(m_globalMasks);
		UpdateHeightCache();
		++m_generationRevision;
		return reset;
	}

	bool ChunkManager::StartThermalErosion()
	{
		if (m_terrainComputePipeline->ResetHydrology(m_globalMasks)) UpdateHeightCache();
		return m_terrainComputePipeline->StartThermalErosion(m_globalMasks);
	}

	void ChunkManager::PauseThermalErosion()
	{
		m_terrainComputePipeline->PauseThermalErosion();
		m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
		UpdateHeightCache();
		++m_generationRevision;
	}

	bool ChunkManager::StepThermalErosion()
	{
		if (m_terrainComputePipeline->IsHydrologyRunning()) return false;
		if (m_terrainComputePipeline->ResetHydrology(m_globalMasks)) UpdateHeightCache();
		if (!m_terrainComputePipeline->StepThermalErosion(m_globalMasks)) return false;
		m_terrainComputePipeline->RefreshMaskPass(m_globalMasks);
		UpdateHeightCache();
		++m_generationRevision;
		return true;
	}

	bool ChunkManager::ResetThermalErosion()
	{
		if (!m_terrainComputePipeline->ResetThermalErosionState(m_globalMasks)) return false;
		UpdateHeightCache();
		++m_generationRevision;
		return true;
	}

	void ChunkManager::ConfigureTerrain(int chunkCount, int chunkSize)
	{
		CancelHydrology();
		m_terrainComputePipeline->CancelParticleErosion();
		m_terrainComputePipeline->PauseThermalErosion();
		ResetHeightCacheReadbacks();
		chunkCount = std::clamp(chunkCount, 3, 31);
		chunkSize = std::clamp(chunkSize, 33, 257);
		if (chunkCount == m_chunkCount && chunkSize == m_terrainSettings.chunkSize)
		{
			return;
		}

		// Preserve all tuned pass parameters (noise / mountains / erosion) across the
		// pipeline rebuild so changing the map size does not reset the user's terrain settings.
		struct SavedParam { ParameterType type; double value; };
		std::unordered_map<std::string, std::unordered_map<std::string, SavedParam>> savedParams;
		TerrainGraphAsset savedUniversalGraph;
		bool hasSavedUniversalGraph = false;
		if (m_terrainComputePipeline)
		{
			savedUniversalGraph = m_terrainComputePipeline->GetUniversalGraphAsset();
			hasSavedUniversalGraph = true;
			for (const auto& pass : m_terrainComputePipeline->GetGraph()->GetPasses())
			{
				for (const auto& p : pass->GetParameters())
				{
					double v = 0.0;
					if (p.type == ParameterType::Float) v = *static_cast<float*>(p.data);
					else if (p.type == ParameterType::Int || p.type == ParameterType::List) v = *static_cast<int*>(p.data);
					else if (p.type == ParameterType::Bool) v = (*static_cast<bool*>(p.data)) ? 1.0 : 0.0;
					savedParams[pass->GetName()][p.name] = { p.type, v };
				}
			}
		}

		m_chunkCount = chunkCount;
		m_terrainSettings.chunkSize = chunkSize;
		m_worldSize = m_terrainSettings.chunkSize * m_chunkCount;
		m_terrainComputePipeline = std::make_shared<TerrainComputePipeline>(m_worldSize, m_resourceManager->GetShaderManager(), m_terrainDebug);
		if (hasSavedUniversalGraph)
		{
			m_terrainComputePipeline->GetUniversalGraphAsset() = savedUniversalGraph;
		}
		const LODManager::Settings lodSettings = m_lodManager->GetSettings();		// D-05: the LOD controls survive the rebuild
		m_lodManager = std::make_unique<LODManager>(m_terrainSettings.chunkSize);
		m_lodManager->SetSettings(lodSettings);
		m_lodManager->GenerateLODMeshes();
		m_globalMasks = m_terrainComputePipeline->CreateMasks();
		m_proceduralAssetsInitialized = false;

		m_terrainDebug->Clear();
		m_terrainDebug->RegisterTexture("NONE", DebugMode::DEBUG_NONE, nullptr, false);
		m_terrainDebug->RegisterTexture("Height", DebugMode::DEBUG_HEIGHT, m_globalMasks["FinalHeight"]->texture, true);
		m_terrainDebug->RegisterTexture("Normal", DebugMode::DEBUG_NORMAL, nullptr, false);
		m_terrainDebug->RegisterTexture("Slope", DebugMode::DEBUG_SLOPE, nullptr, false);
		m_terrainDebug->RegisterTexture("Water", DebugMode::DEBUG_WATER, m_globalMasks["WaterMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Sediment", DebugMode::DEBUG_SEDIMENT, m_globalMasks["SedimentMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Deposited Sediment", DebugMode::DEBUG_DEPOSIT, m_globalMasks["DepositMask"]->texture, true);
		m_terrainDebug->RegisterTexture("Generated Flow", DebugMode::DEBUG_WATER_FLUX, m_globalMasks["FlowAccumulationMask"]->texture, true);
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

		// Restore the saved pass parameters into the freshly built graph.
		for (const auto& pass : m_terrainComputePipeline->GetGraph()->GetPasses())
		{
			auto passIt = savedParams.find(pass->GetName());
			if (passIt == savedParams.end())
			{
				continue;
			}
			for (const auto& p : pass->GetParameters())
			{
				auto paramIt = passIt->second.find(p.name);
				if (paramIt == passIt->second.end())
				{
					continue;
				}
				if (p.type == ParameterType::Float) *static_cast<float*>(p.data) = static_cast<float>(paramIt->second.value);
				else if (p.type == ParameterType::Int || p.type == ParameterType::List) *static_cast<int*>(p.data) = static_cast<int>(paramIt->second.value);
				else if (p.type == ParameterType::Bool) *static_cast<bool*>(p.data) = paramIt->second.value > 0.5;
			}
		}

		m_heightCache.clear();
		m_heightCacheValid = false;
		m_needRegenerate = true;
		++m_generationRevision;
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

	std::uint64_t ChunkManager::GetGenerationRevision() const
	{
		return m_generationRevision;
	}

	bool ChunkManager::IsParticleErosionActive() const
	{
		return m_terrainComputePipeline->IsParticleErosionActive();
	}

	bool ChunkManager::IsParticleErosionReady() const { return m_terrainComputePipeline->IsParticleErosionReady(); }
	float ChunkManager::GetParticleErosionSpeed() const { return m_terrainComputePipeline->GetParticleErosionSpeed(); }
	void ChunkManager::SetParticleErosionSpeed(float speed) { m_terrainComputePipeline->SetParticleErosionSpeed(speed); }
	bool ChunkManager::GetShowErosionParticles() const { return m_terrainComputePipeline->GetShowErosionParticles(); }
	void ChunkManager::SetShowErosionParticles(bool show) { m_terrainComputePipeline->SetShowErosionParticles(show); }
	int ChunkManager::GetActiveParticleCount() const { return m_terrainComputePipeline->GetActiveParticleCount(); }
	bool ChunkManager::IsThermalErosionActive() const { return m_terrainComputePipeline->IsThermalErosionActive(); }
	bool ChunkManager::IsThermalErosionReady() const { return m_terrainComputePipeline->IsThermalErosionReady(); }
	bool ChunkManager::IsThermalErosionStable() const { return m_terrainComputePipeline->IsThermalErosionStable(); }
	float ChunkManager::GetThermalErosionSpeed() const { return m_terrainComputePipeline->GetThermalErosionSpeed(); }
	void ChunkManager::SetThermalErosionSpeed(float speed) { m_terrainComputePipeline->SetThermalErosionSpeed(speed); }
	std::string ChunkManager::GetThermalErosionStatus() const { return m_terrainComputePipeline->GetThermalErosionStatus(); }
	float ChunkManager::SampleHeightAtWorld(float worldX, float worldZ)
	{
		if (!m_heightCacheValid || m_heightCache.empty() || m_worldSize <= 1)
		{
			return 0.0f;
		}

		float worldHalfSize = (m_worldSize - 1) * 0.5f;
		// Chunk UVs map global grid coordinate g to (g + 0.5) / worldSize, whose
		// OpenGL texel coordinate is exactly g. Match that mapping on the CPU.
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
		// Synchronous readback of FinalHeight (design F8): waits for the GPU and the transfer.
		PWG_SIM_EVENT(readbackEvent, debug::SimEventType::HEIGHT_READBACK_SYNC);
		PWG_SIM_EVENT_SET(readbackEvent, resolution, m_worldSize);
		auto heightMask = m_globalMasks.find("FinalHeight");
		if (heightMask == m_globalMasks.end() || !heightMask->second || !heightMask->second->texture)
		{
			m_heightCacheValid = false;
			return;
		}

		ResetHeightCacheReadbacks();
		PublishHeightCache(heightMask->second->texture->ReadFloatData());
	}

	void ChunkManager::PublishHeightCache(std::vector<float>&& values)
	{
		m_heightCache = std::move(values);
		m_heightCacheValid = m_heightCache.size() >= static_cast<size_t>(m_worldSize) * static_cast<size_t>(m_worldSize);
		if (m_heightCacheValid && !m_heightCache.empty())
		{
			auto [minimum, maximum] = std::minmax_element(m_heightCache.begin(), m_heightCache.end());
			m_terrainHeightMin = *minimum;
			m_terrainHeightMax = std::max(*maximum, m_terrainHeightMin + 0.001f);
			m_terrainMaterial->SetUniformFloat("u_terrainHeightMin", m_terrainHeightMin);
			m_terrainMaterial->SetUniformFloat("u_terrainHeightMax", m_terrainHeightMax);
		}
		m_heightCacheAccumulator = 0.0f;
	}

	void ChunkManager::QueueHeightCacheReadback()
	{
		auto heightMask = m_globalMasks.find("FinalHeight");
		if (heightMask == m_globalMasks.end() || !heightMask->second || !heightMask->second->texture || m_worldSize <= 1) return;

		HeightReadbackSlot& slot = m_heightReadbacks[m_nextHeightReadback];
		if (slot.pending) return;
		PWG_SIM_EVENT(queueEvent, debug::SimEventType::HEIGHT_READBACK_QUEUE);
		PWG_SIM_EVENT_SET(queueEvent, resolution, m_worldSize);
		slot.queuedFrame = UINT64_MAX;
		PWG_SIM_EVENT_DO(queueEvent, slot.queuedFrame = queueEvent.Event().frameIndex);
		const std::size_t byteSize = static_cast<std::size_t>(m_worldSize) * static_cast<std::size_t>(m_worldSize) * sizeof(float);
		if (slot.buffer == 0) glGenBuffers(1, &slot.buffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
		if (slot.byteSize != byteSize)
		{
			glBufferData(GL_PIXEL_PACK_BUFFER, byteSize, nullptr, GL_STREAM_READ);
			slot.byteSize = byteSize;
		}
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glBindTexture(GL_TEXTURE_2D, heightMask->second->texture->GetTextureID());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		slot.pending = true;
		m_nextHeightReadback = (m_nextHeightReadback + 1u) % m_heightReadbacks.size();
	}

	bool ChunkManager::PollHeightCacheReadback()
	{
		for (auto& slot : m_heightReadbacks)
		{
			if (!slot.pending || !slot.fence) continue;
			const GLenum result = glClientWaitSync(static_cast<GLsync>(slot.fence), 0, 0);
			if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED) continue;

			PWG_SIM_EVENT(publishEvent, debug::SimEventType::HEIGHT_READBACK_PUBLISH);
			PWG_SIM_EVENT_DO(publishEvent,
				publishEvent.Event().resolution = m_worldSize;
				if (slot.queuedFrame != UINT64_MAX)
					publishEvent.Event().latencyFrames = static_cast<int64_t>(publishEvent.Event().frameIndex - slot.queuedFrame));
			std::vector<float> values(slot.byteSize / sizeof(float));
			glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.buffer);
			const void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, slot.byteSize, GL_MAP_READ_BIT);
			if (mapped) std::memcpy(values.data(), mapped, slot.byteSize);
			if (mapped) glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			glDeleteSync(static_cast<GLsync>(slot.fence));
			slot.fence = nullptr;
			slot.pending = false;
			PWG_SIM_EVENT_SET(publishEvent, success, mapped != nullptr);
			PWG_SIM_EVENT_END(publishEvent);
			if (!mapped) return false;
			PublishHeightCache(std::move(values));
			return true;
		}
		return false;
	}

	void ChunkManager::ResetHeightCacheReadbacks()
	{
		for (auto& slot : m_heightReadbacks)
		{
			if (slot.fence)
			{
				glClientWaitSync(static_cast<GLsync>(slot.fence), GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
				glDeleteSync(static_cast<GLsync>(slot.fence));
			}
			slot.fence = nullptr;
			slot.pending = false;
		}
	}

	float ChunkManager::GetTerrainHeightMin() const { return m_terrainHeightMin; }
	float ChunkManager::GetTerrainHeightMax() const { return m_terrainHeightMax; }

	bool ChunkManager::MaybeUpdateHeightCache(float dt)
	{
		const bool published = PollHeightCacheReadback();
		m_heightCacheAccumulator += std::clamp(dt, 0.0f, 0.25f);
		if (m_heightCacheAccumulator >= m_heightCacheInterval)
		{
			QueueHeightCacheReadback();
			m_heightCacheAccumulator = 0.0f;
		}
		return published;
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

	void ChunkManager::GenerateWorld()
	{
		m_terrainComputePipeline->GenerateWorld(m_globalMasks);
		if (!m_proceduralAssetsInitialized)
		{
			m_proceduralAssetsInitialized = m_terrainComputePipeline->RefreshProceduralAssets(m_globalMasks);
		}
		if (!m_terrainComputePipeline->IsParticleErosionActive() && !m_terrainComputePipeline->IsThermalErosionActive())
		{
			UpdateHeightCache();
			++m_generationRevision;
		}
		m_needRegenerate = false;
	}

	bool ChunkManager::RunUniversalGraph()
	{
		if (m_terrainComputePipeline->IsParticleErosionActive() || m_terrainComputePipeline->IsThermalErosionActive()) return false;
		// CPU wall time of the whole run, including the synchronous height readback (design F8).
		PWG_SIM_EVENT(runEvent, debug::SimEventType::GRAPH_RUN);
		PWG_SIM_EVENT_SET(runEvent, resolution, m_worldSize);
		CancelHydrology();
		bool executed = m_terrainComputePipeline->ExecuteUniversalGraph(m_globalMasks);
		PWG_SIM_EVENT_SET(runEvent, success, executed);
		if (executed && !m_terrainComputePipeline->IsParticleErosionActive() && !m_terrainComputePipeline->IsThermalErosionActive())
		{
			m_globalMasks["BaseHeight"]->texture->CopyFrom(*m_globalMasks["FinalHeight"]->texture);
			UpdateHeightCache();
			++m_generationRevision;
			m_needRegenerate = false;
		}
		return executed;
	}

	bool ChunkManager::RefreshProceduralAssets()
	{
		if (m_terrainComputePipeline->IsParticleErosionActive() || m_terrainComputePipeline->IsThermalErosionActive()) return false;
		const bool refreshed = m_terrainComputePipeline->RefreshProceduralAssets(m_globalMasks);
		m_proceduralAssetsInitialized = refreshed || m_proceduralAssetsInitialized;
		return refreshed;
	}

	const std::string& ChunkManager::GetProceduralAssetStatus() const
	{
		return m_terrainComputePipeline->GetProceduralAssetStatus();
	}

}


