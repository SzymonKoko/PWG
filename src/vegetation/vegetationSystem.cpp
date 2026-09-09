#include "vegetationSystem.h"

#include "core/logger/logger.h"

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <unordered_map>

namespace pwg
{
	namespace
	{
		std::uint32_t Hash(std::int32_t x, std::int32_t z, std::uint32_t seed)
		{
			std::uint32_t h = seed ^ (static_cast<std::uint32_t>(x) * 0x9e3779b9u) ^ (static_cast<std::uint32_t>(z) * 0x85ebca6bu);
			h ^= h >> 16;
			h *= 0x7feb352du;
			h ^= h >> 15;
			h *= 0x846ca68bu;
			h ^= h >> 16;
			return h;
		}

		float Unit(std::uint32_t value)
		{
			return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x01000000u);
		}

		glm::mat4 SlopeAlignment(const glm::vec3& terrainNormal, float maxTiltDegrees)
		{
			const glm::vec3 up(0.0f, 1.0f, 0.0f);
			glm::vec3 normal = glm::normalize(terrainNormal);
			float angle = std::acos(std::clamp(glm::dot(up, normal), -1.0f, 1.0f));
			float limitedAngle = std::min(angle, glm::radians(std::max(maxTiltDegrees, 0.0f)));
			glm::vec3 axis = glm::cross(up, normal);
			if (limitedAngle <= 0.0001f || glm::dot(axis, axis) <= 0.000001f)
			{
				return glm::mat4(1.0f);
			}
			return glm::rotate(glm::mat4(1.0f), limitedAngle, glm::normalize(axis));
		}

		glm::vec3 MaterialColor(const aiMaterial* material)
		{
			aiColor4D color;
			if (material && material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
			{
				return glm::clamp(glm::vec3(color.r, color.g, color.b), glm::vec3(0.0f), glm::vec3(1.0f));
			}
			aiColor3D diffuse;
			if (material && material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
			{
				return glm::clamp(glm::vec3(diffuse.r, diffuse.g, diffuse.b), glm::vec3(0.0f), glm::vec3(1.0f));
			}
			return glm::vec3(0.72f, 0.78f, 0.68f);
		}

		std::string AlbedoReference(const aiMaterial* material)
		{
			if (!material)
			{
				return {};
			}
			aiString texture;
			if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texture) != AI_SUCCESS &&
				material->GetTexture(aiTextureType_DIFFUSE, 0, &texture) != AI_SUCCESS)
			{
				return {};
			}
			return texture.C_Str();
		}
	}

	VegetationSystem::VegetationSystem(std::shared_ptr<Shader> vegetationShader, std::shared_ptr<Shader> shadowShader, std::shared_ptr<VegetationAssetRegistry> assetRegistry)
		: m_vegetationShader(std::move(vegetationShader)), m_shadowShader(std::move(shadowShader)), m_assetRegistry(std::move(assetRegistry))
	{
		m_status = m_assetRegistry ? "Ready" : "Vegetation asset registry is unavailable";
	}

	bool VegetationSystem::LoadModel(const VegetationAssetDefinition& definition)
	{
		const std::string& path = definition.modelPath;
		if (!m_vegetationShader || !std::filesystem::exists(path))
		{
			PWG_WARN("Vegetation asset missing: {0}", path);
			return false;
		}

		Assimp::Importer importer;
		importer.SetPropertyInteger(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, 1);
		const aiScene* scene = importer.ReadFile(path,
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality |
			aiProcess_FlipUVs |
			aiProcess_PreTransformVertices |
			aiProcess_ValidateDataStructure);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
		{
			PWG_ERROR("Vegetation import failed ({0}): {1}", path, importer.GetErrorString());
			return false;
		}

		VegetationModel model;
		model.assetId = definition.id;
		model.kind = definition.kind;
		model.targetHeight = definition.targetHeight;
		model.groundSink = definition.groundSink;
		model.submeshesAsVariants = definition.submeshesAsVariants;
		model.boundsMin = glm::vec3(std::numeric_limits<float>::max());
		model.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
		std::filesystem::path folder = std::filesystem::path(path).parent_path();
		std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
		std::vector<glm::vec3> groundingPositions;
		glm::mat4 orientationCorrection(1.0f);
		orientationCorrection = glm::rotate(orientationCorrection, glm::radians(definition.rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
		orientationCorrection = glm::rotate(orientationCorrection, glm::radians(definition.rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
		orientationCorrection = glm::rotate(orientationCorrection, glm::radians(definition.rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat3 normalCorrection(orientationCorrection);

		for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
		{
			const aiMesh* source = scene->mMeshes[meshIndex];
			if (!source || source->mNumVertices == 0)
			{
				continue;
			}
			const aiMaterial* sourceMaterial = source->mMaterialIndex < scene->mNumMaterials ? scene->mMaterials[source->mMaterialIndex] : nullptr;
			glm::vec3 baseColor = MaterialColor(sourceMaterial);
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indices;
			glm::vec3 meshBoundsMin(std::numeric_limits<float>::max());
			glm::vec3 meshBoundsMax(std::numeric_limits<float>::lowest());
			vertices.reserve(source->mNumVertices);
			groundingPositions.reserve(groundingPositions.size() + source->mNumVertices);
			for (unsigned int i = 0; i < source->mNumVertices; ++i)
			{
				Vertex vertex{};
				glm::vec3 sourcePosition(source->mVertices[i].x, source->mVertices[i].y, source->mVertices[i].z);
				vertex.position = glm::vec3(orientationCorrection * glm::vec4(sourcePosition, 1.0f));
				vertex.normal = source->HasNormals()
					? glm::normalize(normalCorrection * glm::vec3(source->mNormals[i].x, source->mNormals[i].y, source->mNormals[i].z))
					: glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.uv = source->HasTextureCoords(0)
					? glm::vec2(source->mTextureCoords[0][i].x, source->mTextureCoords[0][i].y)
					: glm::vec2(0.0f);
				vertex.color = source->HasVertexColors(0)
					? glm::vec3(source->mColors[0][i].r, source->mColors[0][i].g, source->mColors[0][i].b)
					: glm::vec3(1.0f);
				model.boundsMin = glm::min(model.boundsMin, vertex.position);
				model.boundsMax = glm::max(model.boundsMax, vertex.position);
				meshBoundsMin = glm::min(meshBoundsMin, vertex.position);
				meshBoundsMax = glm::max(meshBoundsMax, vertex.position);
				groundingPositions.push_back(vertex.position);
				vertices.push_back(vertex);
			}
			for (unsigned int faceIndex = 0; faceIndex < source->mNumFaces; ++faceIndex)
			{
				const aiFace& face = source->mFaces[faceIndex];
				if (face.mNumIndices == 3)
				{
					indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
				}
			}
			if (indices.empty())
			{
				continue;
			}

			VegetationSubmesh submesh;
			submesh.mesh = std::make_shared<Mesh>(vertices, indices, static_cast<float>(vertices.size()));
			submesh.boundsMin = meshBoundsMin;
			submesh.boundsMax = meshBoundsMax;
			float meshHeight = meshBoundsMax.y - meshBoundsMin.y;
			if (meshHeight > 0.0001f)
			{
				glm::vec2 meshCenter((meshBoundsMin.x + meshBoundsMax.x) * 0.5f, (meshBoundsMin.z + meshBoundsMax.z) * 0.5f);
				float baseLimit = meshBoundsMin.y + meshHeight * 0.06f;
				float groundRadius = 0.0f;
				float footprintRadius = 0.0f;
				for (const Vertex& vertex : vertices)
				{
					float horizontalRadius = glm::distance(glm::vec2(vertex.position.x, vertex.position.z), meshCenter);
					footprintRadius = std::max(footprintRadius, horizontalRadius);
					if (vertex.position.y <= baseLimit) groundRadius = std::max(groundRadius, horizontalRadius);
				}
				submesh.groundRadiusRatio = std::max(groundRadius / meshHeight, 0.01f);
				submesh.footprintRadiusRatio = std::max(footprintRadius / meshHeight, 0.01f);
			}
			submesh.material = std::make_shared<Material>(m_vegetationShader);
			submesh.material->SetUniformVec3("u_baseColor", baseColor);
			std::string albedoReference = AlbedoReference(sourceMaterial);
			if (!albedoReference.empty() && albedoReference.front() == '*')
			{
				std::string cacheKey = path + "#" + albedoReference;
				auto found = textureCache.find(cacheKey);
				if (found == textureCache.end())
				{
					const aiTexture* embedded = scene->GetEmbeddedTexture(albedoReference.c_str());
					if (embedded && embedded->mHeight == 0 && embedded->mWidth > 0)
					{
						const auto* bytes = reinterpret_cast<const unsigned char*>(embedded->pcData);
						found = textureCache.emplace(cacheKey, std::make_shared<Texture>(bytes, embedded->mWidth, TextureType::ALBEDO)).first;
					}
				}
				if (found != textureCache.end()) submesh.albedo = found->second;
			}
			else if (!albedoReference.empty())
			{
				std::filesystem::path albedoPath = albedoReference;
				if (!albedoPath.is_absolute()) albedoPath = folder / albedoPath;
				std::string resolved = albedoPath.lexically_normal().generic_string();
				if (std::filesystem::exists(resolved))
				{
					auto found = textureCache.find(resolved);
					if (found == textureCache.end()) found = textureCache.emplace(resolved, std::make_shared<Texture>(resolved, TextureType::ALBEDO)).first;
					submesh.albedo = found->second;
				}
			}
			if (submesh.albedo) submesh.material->SetUniformTexture("u_albedo", submesh.albedo);
			submesh.material->SetUniformInt("u_hasAlbedo", submesh.albedo ? 1 : 0);
			model.submeshes.push_back(std::move(submesh));
		}

		float sourceHeight = model.boundsMax.y - model.boundsMin.y;
		if (sourceHeight > 0.0001f)
		{
			glm::vec2 center((model.boundsMin.x + model.boundsMax.x) * 0.5f, (model.boundsMin.z + model.boundsMax.z) * 0.5f);
			float baseLimit = model.boundsMin.y + sourceHeight * 0.06f;
			float groundRadius = 0.0f;
			float footprintRadius = 0.0f;
			for (const glm::vec3& position : groundingPositions)
			{
				float horizontalRadius = glm::distance(glm::vec2(position.x, position.z), center);
				footprintRadius = std::max(footprintRadius, horizontalRadius);
				if (position.y <= baseLimit)
				{
					groundRadius = std::max(groundRadius, horizontalRadius);
				}
			}
			model.groundRadiusRatio = std::max(groundRadius / sourceHeight, 0.01f);
			model.footprintRadiusRatio = std::max(footprintRadius / sourceHeight, 0.01f);
		}
		model.loaded = !model.submeshes.empty() && sourceHeight > 0.0001f;
		if (!model.loaded)
		{
			return false;
		}
		int modelIndex = static_cast<int>(m_models.size());
		m_modelIndices[definition.id] = modelIndex;
		m_models.push_back(std::move(model));
		PWG_INFO("Vegetation asset loaded: {0} ({1} submeshes)", path, m_models.back().submeshes.size());
		return true;
	}

	int VegetationSystem::EnsureModel(const std::string& assetId)
	{
		auto loaded = m_modelIndices.find(assetId);
		if (loaded != m_modelIndices.end())
		{
			return loaded->second;
		}
		const VegetationAssetDefinition* definition = m_assetRegistry ? m_assetRegistry->Find(assetId) : nullptr;
		if (!definition)
		{
			m_status = "Missing vegetation asset: " + assetId;
			PWG_WARN("Missing vegetation asset definition: {0}", assetId);
			return -1;
		}
		if (!LoadModel(*definition))
		{
			m_status = "Failed to load vegetation asset: " + definition->name;
			return -1;
		}
		return m_modelIndices[assetId];
	}

	void VegetationSystem::Sync(std::shared_ptr<Terrain> terrain, const TerrainComputePipeline& pipeline, std::uint64_t terrainRevision)
	{
		if (!terrain)
		{
			return;
		}
		if (m_seenGraphRevision == pipeline.GetVegetationRevision() && m_seenTerrainRevision == terrainRevision)
		{
			return;
		}
		m_seenGraphRevision = pipeline.GetVegetationRevision();
		m_seenTerrainRevision = terrainRevision;
		Regenerate(terrain, pipeline.GetVegetationLayers());
	}

	void VegetationSystem::Regenerate(std::shared_ptr<Terrain> terrain, const std::vector<VegetationLayerDescriptor>& layers)
	{
		m_layers.clear();
		m_trees.clear();
		m_grass.clear();
		m_lastGrassCamera = glm::vec3(1000000.0f);
		m_grassBuilt = false;
		m_worldSize = terrain ? terrain->GetSize() : 0;
		if (!terrain || m_worldSize <= 1)
		{
			return;
		}
		auto chunkManager = terrain->GetChunkManager();
		m_minVisibleWaterDepth = chunkManager->GetTerrainComputePipeline()->GetHydraulicSettings().minVisibleWaterDepth;
		m_worldMin = -(static_cast<float>(m_worldSize) - 1.0f) * 0.5f;
		m_worldMax = m_worldMin + static_cast<float>(chunkManager->GetChunkCount() * (chunkManager->GetChunkSize() - 1));

		std::unordered_map<unsigned int, std::shared_ptr<std::vector<float>>> maskCache;
		std::size_t missingAssetCount = 0;
		for (const auto& descriptor : layers)
		{
			RuntimeLayer layer;
			layer.descriptor = descriptor;
			layer.modelIndex = EnsureModel(descriptor.assetId);
			if (layer.modelIndex < 0) ++missingAssetCount;
			if (descriptor.densityMask && descriptor.densityMask->texture)
			{
				unsigned int textureId = descriptor.densityMask->texture->GetTextureID();
				auto found = maskCache.find(textureId);
				if (found == maskCache.end())
				{
					auto data = std::make_shared<std::vector<float>>(descriptor.densityMask->texture->ReadFloatData());
					found = maskCache.emplace(textureId, std::move(data)).first;
				}
				layer.densityData = found->second;
			}
			m_layers.push_back(std::move(layer));
		}

		auto& masks = terrain->GetChunkManager()->GetMasks();
		auto water = masks.find("WaterMask");
		m_waterData = water != masks.end() && water->second && water->second->texture
			? water->second->texture->ReadFloatData()
			: std::vector<float>{};

		const float worldExtent = m_worldMax - m_worldMin;
		constexpr float collisionCellSize = 25.0f;
		std::unordered_map<long long, std::vector<glm::vec2>> accepted;
		auto key = [](int x, int z)
		{
			return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(z);
		};

		for (const auto& layer : m_layers)
		{
			const VegetationModel* model = FindModel(layer.modelIndex);
			if (!model || model->kind == VegetationKind::Grass)
			{
				continue;
			}
			float spacing = std::max(layer.descriptor.spacing, 10.0f);
			int cells = static_cast<int>(std::ceil(worldExtent / spacing));
			for (int z = 0; z < cells && m_trees.size() < 5000; ++z)
			{
				for (int x = 0; x < cells && m_trees.size() < 5000; ++x)
				{
					std::uint32_t hash = Hash(x, z, static_cast<std::uint32_t>(layer.descriptor.seed ^ (layer.descriptor.nodeId * 7919)));
					float worldX = m_worldMin + (static_cast<float>(x) + 0.1f + Unit(hash) * 0.8f) * spacing;
					float worldZ = m_worldMin + (static_cast<float>(z) + 0.1f + Unit(Hash(x, z, hash ^ 0xa511e9b3u)) * 0.8f) * spacing;
					float scale = glm::mix(layer.descriptor.minScale, layer.descriptor.maxScale, Unit(Hash(x, z, hash ^ 0xc2b2ae35u)));
					int variantIndex = model->submeshesAsVariants ? static_cast<int>(hash % model->submeshes.size()) : -1;
					float height = model->targetHeight * scale;
					float groundRadiusRatio = variantIndex >= 0 ? model->submeshes[static_cast<std::size_t>(variantIndex)].groundRadiusRatio : model->groundRadiusRatio;
					float footprintRadiusRatio = variantIndex >= 0 ? model->submeshes[static_cast<std::size_t>(variantIndex)].footprintRadiusRatio : model->footprintRadiusRatio;
					float minimumGroundRadius = model->kind == VegetationKind::Rock ? 0.25f : 0.6f;
					float groundRadius = std::clamp(groundRadiusRatio * height, minimumGroundRadius, 2.5f);
					float footprintRadius = footprintRadiusRatio * height;
					if (worldX - footprintRadius < m_worldMin || worldX + footprintRadius > m_worldMax ||
						worldZ - footprintRadius < m_worldMin || worldZ + footprintRadius > m_worldMax)
					{
						continue;
					}
					float mask = layer.densityData ? SampleMask(*layer.densityData, worldX, worldZ) : 1.0f;
					if (Unit(Hash(x, z, hash ^ 0x63d83595u)) > layer.descriptor.density * std::clamp(mask, 0.0f, 1.0f) ||
						!AcceptTerrainPoint(terrain, worldX, worldZ, layer.descriptor.maxSlopeDegrees, model->kind != VegetationKind::Rock))
					{
						continue;
					}

					float minimumDistance = spacing * 0.65f;
					int cellX = static_cast<int>(std::floor((worldX - m_worldMin) / collisionCellSize));
					int cellZ = static_cast<int>(std::floor((worldZ - m_worldMin) / collisionCellSize));
					int range = static_cast<int>(std::ceil(minimumDistance / collisionCellSize));
					bool tooClose = false;
					for (int dz = -range; dz <= range && !tooClose; ++dz)
					{
						for (int dx = -range; dx <= range && !tooClose; ++dx)
						{
							auto found = accepted.find(key(cellX + dx, cellZ + dz));
							if (found == accepted.end()) continue;
							for (const glm::vec2& point : found->second)
							{
								if (glm::distance(point, glm::vec2(worldX, worldZ)) < minimumDistance)
								{
									tooClose = true;
									break;
								}
							}
						}
					}
					if (tooClose) continue;

					accepted[key(cellX, cellZ)].push_back(glm::vec2(worldX, worldZ));
					float yaw = Unit(Hash(x, z, hash ^ 0x27d4eb2fu)) * glm::two_pi<float>();
					float groundSink = std::max(model->groundSink * scale, 0.0f);
					glm::vec3 position(worldX, SampleGroundHeight(terrain, worldX, worldZ, groundRadius, groundSink), worldZ);
					glm::vec3 normal = layer.descriptor.alignToSlope ? SampleTerrainNormal(terrain, worldX, worldZ) : glm::vec3(0.0f, 1.0f, 0.0f);
					glm::mat4 modelMatrix = variantIndex >= 0
						? BuildModelMatrix(model->submeshes[static_cast<std::size_t>(variantIndex)], model->targetHeight, position, yaw, scale, normal, layer.descriptor.maxTiltDegrees)
						: BuildModelMatrix(*model, position, yaw, scale, normal, layer.descriptor.maxTiltDegrees);
					float collisionRadius = model->kind == VegetationKind::Rock ? std::max(footprintRadius, 0.35f) : 0.0f;
					float collisionHeight = model->kind == VegetationKind::Rock ? std::max(height - groundSink, 0.5f) : 0.0f;
					m_trees.push_back({ layer.modelIndex, position, modelMatrix, height * 0.6f, layer.descriptor.renderDistance, groundRadius, groundSink, collisionRadius, collisionHeight, variantIndex });
				}
			}
		}
		m_status = "Generated " + std::to_string(m_trees.size()) + " static objects";
		if (missingAssetCount > 0)
		{
			m_status += "; missing " + std::to_string(missingAssetCount) + " asset" + (missingAssetCount == 1 ? "" : "s");
		}
	}

	void VegetationSystem::Update(std::shared_ptr<Terrain> terrain, std::shared_ptr<ICamera> camera, float dt)
	{
		m_time += std::clamp(dt, 0.0f, 0.1f);
		m_groundingAccumulator += std::clamp(dt, 0.0f, 0.1f);
		if (!terrain || !camera)
		{
			return;
		}
		glm::vec3 cameraPosition = camera->GetCameraPosition();
		m_minVisibleWaterDepth = terrain->GetChunkManager()->GetTerrainComputePipeline()->GetHydraulicSettings().minVisibleWaterDepth;
		float grassRebuildDistance = std::numeric_limits<float>::max();
		for (const auto& layer : m_layers)
		{
			const VegetationModel* model = FindModel(layer.modelIndex);
			if (model && model->kind == VegetationKind::Grass)
			{
				grassRebuildDistance = std::min(grassRebuildDistance, std::clamp(layer.descriptor.renderDistance * 0.2f, 24.0f, 80.0f));
			}
		}
		if (grassRebuildDistance < std::numeric_limits<float>::max() &&
			(!m_grassBuilt || glm::distance(glm::vec2(cameraPosition.x, cameraPosition.z), glm::vec2(m_lastGrassCamera.x, m_lastGrassCamera.z)) > grassRebuildDistance))
		{
			RebuildGrass(terrain, cameraPosition);
		}
		if (m_groundingAccumulator >= 0.25f)
		{
			RefreshGrounding(terrain);
			m_groundingAccumulator = 0.0f;
		}
	}

	void VegetationSystem::RebuildGrass(std::shared_ptr<Terrain> terrain, const glm::vec3& cameraPosition)
	{
		constexpr std::size_t maxGrassInstances = 120000;
		m_grass.clear();
		m_lastGrassCamera = cameraPosition;
		m_grassBuilt = true;
		std::size_t remainingGrassLayers = static_cast<std::size_t>(std::count_if(m_layers.begin(), m_layers.end(), [&](const RuntimeLayer& layer)
		{
			const VegetationModel* model = FindModel(layer.modelIndex);
			return model && model->kind == VegetationKind::Grass;
		}));
		for (const auto& layer : m_layers)
		{
			const VegetationModel* model = FindModel(layer.modelIndex);
			if (!model || model->kind != VegetationKind::Grass)
			{
				continue;
			}
			if (remainingGrassLayers == 0) continue;
			std::size_t layerLimit = m_grass.size() + (maxGrassInstances - m_grass.size()) / remainingGrassLayers;
			--remainingGrassLayers;
			float spacing = std::max(layer.descriptor.spacing, 1.0f);
			float radius = layer.descriptor.renderDistance;
			int centerX = static_cast<int>(std::floor((cameraPosition.x - m_worldMin) / spacing));
			int centerZ = static_cast<int>(std::floor((cameraPosition.z - m_worldMin) / spacing));
			int maxRing = static_cast<int>(std::ceil(radius / spacing)) + 1;

			auto tryCell = [&](int x, int z)
			{
				if (m_grass.size() >= layerLimit) return;
				std::uint32_t hash = Hash(x, z, static_cast<std::uint32_t>(layer.descriptor.seed ^ (layer.descriptor.nodeId * 7919)));
				float worldX = m_worldMin + (static_cast<float>(x) + 0.1f + Unit(hash) * 0.8f) * spacing;
				float worldZ = m_worldMin + (static_cast<float>(z) + 0.1f + Unit(Hash(x, z, hash ^ 0x517cc1b7u)) * 0.8f) * spacing;
				int variantIndex = model->submeshesAsVariants ? static_cast<int>(hash % model->submeshes.size()) : -1;
				float scale = glm::mix(layer.descriptor.minScale, layer.descriptor.maxScale, Unit(Hash(x, z, hash ^ 0xb5297a4du)));
				float height = model->targetHeight * scale;
				float groundRadiusRatio = variantIndex >= 0 ? model->submeshes[static_cast<std::size_t>(variantIndex)].groundRadiusRatio : model->groundRadiusRatio;
				float footprintRadiusRatio = variantIndex >= 0 ? model->submeshes[static_cast<std::size_t>(variantIndex)].footprintRadiusRatio : model->footprintRadiusRatio;
				float groundRadius = std::clamp(groundRadiusRatio * height, 0.25f, 1.6f);
				float footprintRadius = footprintRadiusRatio * height;
				if (worldX - footprintRadius < m_worldMin || worldX + footprintRadius > m_worldMax ||
					worldZ - footprintRadius < m_worldMin || worldZ + footprintRadius > m_worldMax ||
					glm::distance(glm::vec2(worldX, worldZ), glm::vec2(cameraPosition.x, cameraPosition.z)) > radius)
				{
					return;
				}
				float mask = layer.densityData ? SampleMask(*layer.densityData, worldX, worldZ) : 1.0f;
				float distribution = layer.densityData
					? glm::mix(0.55f, 1.0f, glm::smoothstep(0.05f, 0.68f, std::clamp(mask, 0.0f, 1.0f)))
					: 1.0f;
				if (Unit(Hash(x, z, hash ^ 0x68e31da4u)) > layer.descriptor.density * distribution ||
					!AcceptTerrainPoint(terrain, worldX, worldZ, layer.descriptor.maxSlopeDegrees, true))
				{
					return;
				}
				float yaw = Unit(Hash(x, z, hash ^ 0x1b56c4e9u)) * glm::two_pi<float>();
				float groundSink = std::max(model->groundSink * scale, 0.0f);
				glm::vec3 position(worldX, SampleGroundHeight(terrain, worldX, worldZ, groundRadius, groundSink), worldZ);
				glm::vec3 normal = layer.descriptor.alignToSlope ? SampleTerrainNormal(terrain, worldX, worldZ) : glm::vec3(0.0f, 1.0f, 0.0f);
				glm::mat4 modelMatrix = variantIndex >= 0
					? BuildModelMatrix(model->submeshes[static_cast<std::size_t>(variantIndex)], model->targetHeight, position, yaw, scale, normal, layer.descriptor.maxTiltDegrees)
					: BuildModelMatrix(*model, position, yaw, scale, normal, layer.descriptor.maxTiltDegrees);
				m_grass.push_back({ layer.modelIndex, position, modelMatrix, height * 0.75f, radius, groundRadius, groundSink, 0.0f, 0.0f, variantIndex });
			};

			tryCell(centerX, centerZ);
			for (int ring = 1; ring <= maxRing && m_grass.size() < layerLimit; ++ring)
			{
				int minX = centerX - ring;
				int maxX = centerX + ring;
				int minZ = centerZ - ring;
				int maxZ = centerZ + ring;
				for (int x = minX; x <= maxX && m_grass.size() < layerLimit; ++x)
				{
					tryCell(x, minZ);
					if (maxZ != minZ) tryCell(x, maxZ);
				}
				for (int z = minZ + 1; z < maxZ && m_grass.size() < layerLimit; ++z)
				{
					tryCell(minX, z);
					if (maxX != minX) tryCell(maxX, z);
				}
			}
		}
	}

	void VegetationSystem::RefreshGrounding(std::shared_ptr<Terrain> terrain)
	{
		if (!terrain) return;
		auto refresh = [&](std::vector<Instance>& instances)
		{
			for (auto& instance : instances)
			{
				float groundedY = SampleGroundHeight(terrain, instance.position.x, instance.position.z, instance.groundRadius, instance.groundSink);
				float delta = groundedY - instance.position.y;
				if (std::abs(delta) > 0.001f)
				{
					instance.position.y = groundedY;
					instance.model[3].y += delta;
				}
			}
		};
		refresh(m_trees);
		refresh(m_grass);
	}

	float VegetationSystem::SampleGroundHeight(std::shared_ptr<Terrain> terrain, float x, float z, float radius, float sink) const
	{
		float height = terrain->GetTerrainHeightAt(x, z);
		constexpr int sampleCount = 8;
		for (int i = 0; i < sampleCount; ++i)
		{
			float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(sampleCount);
			float sampleX = x + std::cos(angle) * radius;
			float sampleZ = z + std::sin(angle) * radius;
			height = std::min(height, terrain->GetTerrainHeightAt(sampleX, sampleZ));
		}
		return height - sink;
	}

	float VegetationSystem::SampleMask(const std::vector<float>& data, float worldX, float worldZ) const
	{
		if (data.size() < static_cast<std::size_t>(m_worldSize) * m_worldSize)
		{
			return 1.0f;
		}
		float half = (static_cast<float>(m_worldSize) - 1.0f) * 0.5f;
		float px = std::clamp(worldX + half, 0.0f, static_cast<float>(m_worldSize - 1));
		float pz = std::clamp(worldZ + half, 0.0f, static_cast<float>(m_worldSize - 1));
		int x0 = static_cast<int>(std::floor(px));
		int z0 = static_cast<int>(std::floor(pz));
		int x1 = std::min(x0 + 1, m_worldSize - 1);
		int z1 = std::min(z0 + 1, m_worldSize - 1);
		float tx = px - x0;
		float tz = pz - z0;
		auto at = [&](int x, int z) { return data[static_cast<std::size_t>(z) * m_worldSize + x]; };
		return glm::mix(glm::mix(at(x0, z0), at(x1, z0), tx), glm::mix(at(x0, z1), at(x1, z1), tx), tz);
	}

	bool VegetationSystem::AcceptTerrainPoint(std::shared_ptr<Terrain> terrain, float x, float z, float maxSlopeDegrees, bool avoidWater) const
	{
		if (avoidWater && !m_waterData.empty() && SampleMask(m_waterData, x, z) >= m_minVisibleWaterDepth)
		{
			return false;
		}
		float dx = (terrain->GetTerrainHeightAt(x + 1.0f, z) - terrain->GetTerrainHeightAt(x - 1.0f, z)) * 0.5f;
		float dz = (terrain->GetTerrainHeightAt(x, z + 1.0f) - terrain->GetTerrainHeightAt(x, z - 1.0f)) * 0.5f;
		float slope = glm::degrees(std::atan(std::sqrt(dx * dx + dz * dz)));
		return slope <= maxSlopeDegrees;
	}

	glm::vec3 VegetationSystem::SampleTerrainNormal(std::shared_ptr<Terrain> terrain, float x, float z) const
	{
		constexpr float offset = 1.0f;
		float dx = (terrain->GetTerrainHeightAt(x + offset, z) - terrain->GetTerrainHeightAt(x - offset, z)) / (2.0f * offset);
		float dz = (terrain->GetTerrainHeightAt(x, z + offset) - terrain->GetTerrainHeightAt(x, z - offset)) / (2.0f * offset);
		return glm::normalize(glm::vec3(-dx, 1.0f, -dz));
	}

	glm::mat4 VegetationSystem::BuildModelMatrix(const VegetationModel& model, const glm::vec3& position, float yaw, float scale, const glm::vec3& normal, float maxTiltDegrees) const
	{
		float sourceHeight = std::max(model.boundsMax.y - model.boundsMin.y, 0.0001f);
		float normalizedScale = model.targetHeight / sourceHeight * scale;
		glm::vec3 center((model.boundsMin.x + model.boundsMax.x) * 0.5f, model.boundsMin.y, (model.boundsMin.z + model.boundsMax.z) * 0.5f);
		glm::mat4 result(1.0f);
		result = glm::translate(result, position);
		result *= SlopeAlignment(normal, maxTiltDegrees);
		result = glm::rotate(result, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		result = glm::scale(result, glm::vec3(normalizedScale));
		result = glm::translate(result, -center);
		return result;
	}

	glm::mat4 VegetationSystem::BuildModelMatrix(const VegetationSubmesh& submesh, float targetHeight, const glm::vec3& position, float yaw, float scale, const glm::vec3& normal, float maxTiltDegrees) const
	{
		float sourceHeight = std::max(submesh.boundsMax.y - submesh.boundsMin.y, 0.0001f);
		float normalizedScale = targetHeight / sourceHeight * scale;
		glm::vec3 center((submesh.boundsMin.x + submesh.boundsMax.x) * 0.5f, submesh.boundsMin.y, (submesh.boundsMin.z + submesh.boundsMax.z) * 0.5f);
		glm::mat4 result(1.0f);
		result = glm::translate(result, position);
		result *= SlopeAlignment(normal, maxTiltDegrees);
		result = glm::rotate(result, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		result = glm::scale(result, glm::vec3(normalizedScale));
		result = glm::translate(result, -center);
		return result;
	}

	const VegetationModel* VegetationSystem::FindModel(int modelIndex) const
	{
		return modelIndex >= 0 && modelIndex < static_cast<int>(m_models.size()) && m_models[static_cast<std::size_t>(modelIndex)].loaded
			? &m_models[static_cast<std::size_t>(modelIndex)]
			: nullptr;
	}

	float VegetationSystem::GetCollisionGroundHeight(std::shared_ptr<Terrain> terrain, float x, float z) const
	{
		float height = terrain ? terrain->GetTerrainHeightAt(x, z) : 0.0f;
		for (const Instance& instance : m_trees)
		{
			const VegetationModel* model = FindModel(instance.modelIndex);
			if (!model || model->kind != VegetationKind::Rock || instance.collisionRadius <= 0.0f) continue;
			glm::vec2 offset(x - instance.position.x, z - instance.position.z);
			float normalizedDistance = glm::length(offset) / instance.collisionRadius;
			if (normalizedDistance < 1.0f)
			{
				float profile = 1.0f - glm::smoothstep(0.0f, 1.0f, normalizedDistance);
				float surfaceHeight = instance.position.y + instance.collisionHeight * profile;
				height = std::max(height, surfaceHeight);
			}
		}
		return height;
	}

	void VegetationSystem::Draw(Renderer& renderer, std::shared_ptr<Terrain> terrain)
	{
		if (!terrain) return;
		auto chunkManager = terrain->GetChunkManager();
		auto& masks = chunkManager->GetMasks();
		auto water = masks.find("WaterMask");
		auto flow = masks.find("FlowAccumulationMask");
		auto channels = masks.find("RiverChannelMask");
		if (water == masks.end() || flow == masks.end() || channels == masks.end()) return;
		auto& hydraulicSettings = chunkManager->GetTerrainComputePipeline()->GetHydraulicSettings();
		for (int modelIndex = 0; modelIndex < static_cast<int>(m_models.size()); ++modelIndex)
		{
			const VegetationModel* model = FindModel(modelIndex);
			if (!model) continue;
			bool grass = model->kind == VegetationKind::Grass;
			bool tree = model->kind == VegetationKind::Tree;
			float frustumMargin = tree ? 18.0f : (grass ? 6.0f : 10.0f);
			bool variants = model->submeshesAsVariants;
			const auto& source = grass ? m_grass : m_trees;
			std::size_t batchCount = variants ? model->submeshes.size() : 1;
			std::vector<std::vector<glm::mat4>> batches(std::max<std::size_t>(batchCount, 1));
			float renderDistance = 1.0f;
			for (const auto& instance : source)
			{
				if (instance.modelIndex == modelIndex && renderer.IsSphereVisible(instance.position, instance.radius, instance.renderDistance, frustumMargin))
				{
					std::size_t batch = variants
						? static_cast<std::size_t>(std::max(instance.variantIndex, 0)) % batches.size()
						: 0;
					batches[batch].push_back(instance.model);
					renderDistance = std::max(renderDistance, instance.renderDistance);
				}
			}
			for (std::size_t submeshIndex = 0; submeshIndex < model->submeshes.size(); ++submeshIndex)
			{
				const auto& submesh = model->submeshes[submeshIndex];
				submesh.material->SetUniformFloat("u_time", m_time);
				submesh.material->SetUniformInt("u_isGrass", grass ? 1 : 0);
				submesh.material->SetUniformInt("u_isTree", tree ? 1 : 0);
				submesh.material->SetUniformFloat("u_modelMinY", variants ? submesh.boundsMin.y : model->boundsMin.y);
				submesh.material->SetUniformFloat("u_modelMaxY", variants ? submesh.boundsMax.y : model->boundsMax.y);
				submesh.material->SetUniformFloat("u_renderDistance", renderDistance);
				submesh.material->SetUniformTexture("u_Watermap", water->second->texture);
				submesh.material->SetUniformTexture("u_FlowAccumulationmap", flow->second->texture);
				submesh.material->SetUniformTexture("u_RiverChannelmap", channels->second->texture);
				submesh.material->SetUniformFloat("u_waterWorldSize", static_cast<float>(m_worldSize));
				submesh.material->SetUniformFloat("u_minVisibleWaterDepth", hydraulicSettings.minVisibleWaterDepth);
				const auto& transforms = variants ? batches[submeshIndex] : batches[0];
				renderer.DrawMeshInstanced(submesh.mesh, submesh.material, transforms);
			}
		}
	}

	void VegetationSystem::DrawShadow(const glm::mat4& lightSpaceMatrix)
	{
		if (!m_shadowShader || m_trees.empty()) return;
		for (int modelIndex = 0; modelIndex < static_cast<int>(m_models.size()); ++modelIndex)
		{
			const VegetationModel* model = FindModel(modelIndex);
			if (!model || model->kind == VegetationKind::Grass) continue;
			bool variants = model->submeshesAsVariants;
			std::size_t batchCount = variants ? model->submeshes.size() : 1;
			std::vector<std::vector<glm::mat4>> batches(std::max<std::size_t>(batchCount, 1));
			for (const auto& instance : m_trees)
			{
				if (instance.modelIndex != modelIndex) continue;
				std::size_t batch = variants
					? static_cast<std::size_t>(std::max(instance.variantIndex, 0)) % batches.size()
					: 0;
				batches[batch].push_back(instance.model);
			}
			for (std::size_t submeshIndex = 0; submeshIndex < model->submeshes.size(); ++submeshIndex)
			{
				const auto& submesh = model->submeshes[submeshIndex];
				const auto& transforms = variants ? batches[submeshIndex] : batches[0];
				if (transforms.empty()) continue;
				m_shadowShader->ActivateShader();
				m_shadowShader->SetUniformMat4("u_lightSpaceMatrix", lightSpaceMatrix);
				m_shadowShader->SetUniformFloat("u_time", m_time);
				m_shadowShader->SetUniformInt("u_isTree", model->kind == VegetationKind::Tree ? 1 : 0);
				m_shadowShader->SetUniformFloat("u_modelMinY", variants ? submesh.boundsMin.y : model->boundsMin.y);
				m_shadowShader->SetUniformFloat("u_modelMaxY", variants ? submesh.boundsMax.y : model->boundsMax.y);
				m_shadowShader->SetUniformInt("u_hasAlbedo", submesh.albedo ? 1 : 0);
				if (submesh.albedo)
				{
					submesh.albedo->Bind(0);
					m_shadowShader->SetUniformInt("u_albedo", 0);
				}
				submesh.mesh->DrawInstanced(transforms);
			}
		}
	}

	void VegetationSystem::Clear()
	{
		m_layers.clear();
		m_trees.clear();
		m_grass.clear();
		m_waterData.clear();
		m_grassBuilt = false;
		m_lastGrassCamera = glm::vec3(1000000.0f);
		m_worldMin = 0.0f;
		m_worldMax = 0.0f;
		m_groundingAccumulator = 0.0f;
		m_seenGraphRevision = 0;
		m_seenTerrainRevision = 0;
	}

	void VegetationSystem::ReloadAssetModels()
	{
		m_models.clear();
		m_modelIndices.clear();
		m_layers.clear();
		m_trees.clear();
		m_grass.clear();
		m_grassBuilt = false;
		m_lastGrassCamera = glm::vec3(1000000.0f);
		m_seenGraphRevision = std::numeric_limits<std::uint64_t>::max();
		m_seenTerrainRevision = std::numeric_limits<std::uint64_t>::max();
		m_status = "Vegetation asset metadata reloaded";
	}

	std::size_t VegetationSystem::GetTreeCount() const { return m_trees.size(); }
	std::size_t VegetationSystem::GetGrassCount() const { return m_grass.size(); }
	const std::string& VegetationSystem::GetStatus() const { return m_status; }
}
