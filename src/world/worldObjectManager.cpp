#include "worldObjectManager.h"

#include "world/objModelImporter.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cmath>
#include <random>
#include <unordered_map>

namespace pwg
{
	namespace
	{
		void LoadSubmeshTextures(ModelAsset& asset)
		{
			for (auto& submesh : asset.submeshes)
			{
				if (!submesh.albedoPath.empty() && !submesh.albedoTexture && std::filesystem::exists(submesh.albedoPath))
				{
					submesh.albedoTexture = std::make_shared<Texture>(submesh.albedoPath, TextureType::ALBEDO);
				}
			}
		}

		void UpdateAssetBounds(ModelAsset& asset)
		{
			glm::vec3 minBounds(std::numeric_limits<float>::max());
			glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
			bool hasVertices = false;

			for (auto& submesh : asset.submeshes)
			{
				if (!submesh.mesh)
				{
					continue;
				}
				for (const auto& vertex : submesh.mesh->GetVertices())
				{
					minBounds = glm::min(minBounds, vertex.position);
					maxBounds = glm::max(maxBounds, vertex.position);
					hasVertices = true;
				}
			}

			if (!hasVertices && asset.mesh)
			{
				for (const auto& vertex : asset.mesh->GetVertices())
				{
					minBounds = glm::min(minBounds, vertex.position);
					maxBounds = glm::max(maxBounds, vertex.position);
					hasVertices = true;
				}
			}

			if (!hasVertices)
			{
				asset.boundsCenter = glm::vec3(0.0f);
				asset.boundsMin = glm::vec3(-0.5f);
				asset.boundsMax = glm::vec3(0.5f);
				asset.boundsRadius = 1.0f;
				asset.defaultScale = glm::vec3(1.0f);
				asset.pivotOffset = glm::vec3(0.0f);
				return;
			}

			asset.boundsMin = minBounds;
			asset.boundsMax = maxBounds;
			asset.boundsCenter = (minBounds + maxBounds) * 0.5f;
			asset.boundsRadius = glm::max(glm::length(maxBounds - asset.boundsCenter), 0.5f);

			constexpr float targetPreviewRadius = 120.0f;
			float uniformScale = glm::clamp(targetPreviewRadius / asset.boundsRadius, 0.001f, 1000.0f);
			asset.defaultScale = glm::vec3(uniformScale);
			asset.pivotOffset = glm::vec3(
				-asset.boundsCenter.x * uniformScale,
				-minBounds.y * uniformScale,
				-asset.boundsCenter.z * uniformScale);
		}
		void UpdateGroundingPoints(ModelAsset& asset)
		{
			asset.groundingPoints.clear();
			glm::mat4 rotation(1.0f);
			rotation = glm::rotate(rotation, asset.importRotation.x, glm::vec3(1, 0, 0));
			rotation = glm::rotate(rotation, asset.importRotation.y, glm::vec3(0, 1, 0));
			rotation = glm::rotate(rotation, asset.importRotation.z, glm::vec3(0, 0, 1));
			auto forEachVertex = [&](auto&& callback)
			{
				if (!asset.submeshes.empty())
				{
					for (const auto& submesh : asset.submeshes)
					{
						if (!submesh.mesh) continue;
						for (const auto& vertex : submesh.mesh->GetVertices()) callback(vertex.position);
					}
				}
				else if (asset.mesh)
				{
					for (const auto& vertex : asset.mesh->GetVertices()) callback(vertex.position);
				}
			};

			glm::vec2 minimum(std::numeric_limits<float>::max());
			glm::vec2 maximum(std::numeric_limits<float>::lowest());
			bool hasVertices = false;
			forEachVertex([&](const glm::vec3& point)
			{
				glm::vec3 oriented = glm::vec3(rotation * glm::vec4(point, 1.0f));
				minimum = glm::min(minimum, glm::vec2(oriented.x, oriented.z));
				maximum = glm::max(maximum, glm::vec2(oriented.x, oriented.z));
				hasVertices = true;
			});
			if (!hasVertices) return;

			constexpr int gridSize = 8;
			std::array<float, gridSize * gridSize> lowest;
			std::array<glm::vec3, gridSize * gridSize> points;
			lowest.fill(std::numeric_limits<float>::max());
			glm::vec2 span = glm::max(maximum - minimum, glm::vec2(0.0001f));
			forEachVertex([&](const glm::vec3& point)
			{
				glm::vec3 oriented = glm::vec3(rotation * glm::vec4(point, 1.0f));
				glm::vec2 normalized = glm::clamp((glm::vec2(oriented.x, oriented.z) - minimum) / span, glm::vec2(0.0f), glm::vec2(0.9999f));
				int x = static_cast<int>(normalized.x * gridSize);
				int z = static_cast<int>(normalized.y * gridSize);
				int index = z * gridSize + x;
				if (oriented.y < lowest[index])
				{
					lowest[index] = oriented.y;
					points[index] = point;
				}
			});
			for (int i = 0; i < gridSize * gridSize; ++i)
			{
				if (lowest[i] != std::numeric_limits<float>::max()) asset.groundingPoints.push_back(points[i]);
			}
		}
		void SyncLegacyMaterialFields(ModelAsset& asset)
		{
			if (!asset.submeshes.empty())
			{
				asset.mesh = asset.submeshes.front().mesh;
				asset.diffuseColor = asset.submeshes.front().diffuseColor;
				asset.albedoPath = asset.submeshes.front().albedoPath;
				asset.albedoTexture = asset.submeshes.front().albedoTexture;
			}
		}
	}

	WorldObjectManager::WorldObjectManager(std::shared_ptr<Material> objectMaterial)
		: m_objectMaterial(objectMaterial)
	{
	}

	int WorldObjectManager::ReserveAssetId()
	{
		return m_nextAssetId++;
	}

	int WorldObjectManager::ReserveObjectId()
	{
		return m_nextObjectId++;
	}

	ModelAsset* WorldObjectManager::ImportObj(const std::string& path)
	{
		auto existing = std::find_if(m_assets.begin(), m_assets.end(), [&](const ModelAsset& asset)
		{
			return asset.path == path;
		});
		if (existing != m_assets.end())
		{
			m_selectedAssetId = existing->id;
			m_status = "Asset already imported: " + existing->name;
			return &(*existing);
		}

		ModelAsset asset;
		asset.id = ReserveAssetId();
		asset.path = std::filesystem::path(path).generic_string();
		asset.name = std::filesystem::path(path).stem().string();

		auto imported = ObjModelImporter::Load(path);
		asset.submeshes = imported.submeshes;
		asset.mesh = imported.mesh;
		asset.loaded = imported.success;
		asset.status = imported.message;
		asset.diffuseColor = imported.diffuseColor;
		asset.albedoPath = imported.albedoPath;
		LoadSubmeshTextures(asset);
		UpdateAssetBounds(asset);
		UpdateGroundingPoints(asset);
		SyncLegacyMaterialFields(asset);
		if (!asset.albedoPath.empty() && !asset.albedoTexture && std::filesystem::exists(asset.albedoPath))
		{
			asset.albedoTexture = std::make_shared<Texture>(asset.albedoPath, TextureType::ALBEDO);
		}
		m_status = imported.message;

		m_assets.push_back(asset);
		m_selectedAssetId = asset.id;
		return &m_assets.back();
	}

	ModelAsset* WorldObjectManager::AddAssetRecord(const ModelAsset& asset)
	{
		ModelAsset stored = asset;
		LoadSubmeshTextures(stored);
		UpdateAssetBounds(stored);
		stored.defaultScale = asset.defaultScale;
		stored.pivotOffset = asset.pivotOffset;
		stored.importRotation = asset.importRotation;
		stored.defaultSurfaceMode = asset.defaultSurfaceMode;
		UpdateGroundingPoints(stored);
		SyncLegacyMaterialFields(stored);
		if (!stored.albedoPath.empty() && !stored.albedoTexture && std::filesystem::exists(stored.albedoPath))
		{
			stored.albedoTexture = std::make_shared<Texture>(stored.albedoPath, TextureType::ALBEDO);
		}
		m_assets.push_back(stored);
		m_nextAssetId = std::max(m_nextAssetId, asset.id + 1);
		if (m_selectedAssetId == 0)
		{
			m_selectedAssetId = asset.id;
		}
		return &m_assets.back();
	}

	WorldObject* WorldObjectManager::AddObjectRecord(const WorldObject& object)
	{
		m_objects.push_back(object);
		m_nextObjectId = std::max(m_nextObjectId, object.id + 1);
		if (object.selected)
		{
			SelectObject(object.id);
		}
		return &m_objects.back();
	}

	WorldObject* WorldObjectManager::CreateObject(int assetId, const glm::vec3& position)
	{
		ModelAsset* asset = FindAsset(assetId);
		if (!asset)
		{
			m_status = "Select an asset before placing an object";
			return nullptr;
		}

		WorldObject object;
		object.id = ReserveObjectId();
		object.assetId = assetId;
		object.name = asset->name + " " + std::to_string(object.id);
		object.transform.position = position;
		object.transform.scale = asset->defaultScale;
		object.snapToTerrain = asset->defaultSurfaceMode != ObjectSurfaceMode::FreeTransform;
		object.freeTransform = asset->defaultSurfaceMode == ObjectSurfaceMode::FreeTransform;
		object.alignToNormal = asset->defaultSurfaceMode == ObjectSurfaceMode::AlignToNormal;

		m_objects.push_back(object);
		SelectObject(object.id);
		m_status = "Placed object: " + object.name;
		return &m_objects.back();
	}

	WorldObject* WorldObjectManager::DuplicateSelected()
	{
		const WorldObject* selected = GetSelectedObject();
		if (!selected)
		{
			m_status = "No object selected";
			return nullptr;
		}

		WorldObject copy = *selected;
		copy.id = ReserveObjectId();
		copy.name = selected->name + " copy";
		copy.transform.position.x += 4.0f;
		copy.transform.position.z += 4.0f;
		m_objects.push_back(copy);
		SelectObject(copy.id);
		m_status = "Duplicated object";
		return &m_objects.back();
	}

	void WorldObjectManager::DeleteSelected()
	{
		if (m_selectedObjectId == 0)
		{
			return;
		}
		m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), [&](const WorldObject& object)
		{
			return object.id == m_selectedObjectId;
		}), m_objects.end());
		m_selectedObjectId = 0;
		m_status = "Deleted object";
	}

	void WorldObjectManager::Clear()
	{
		m_assets.clear();
		m_objects.clear();
		m_ruleSets.clear();
		m_exclusionZones.clear();
		m_nextAssetId = 1;
		m_nextObjectId = 1;
		m_nextRuleSetId = 1;
		m_nextExclusionZoneId = 1;
		m_selectedAssetId = 0;
		m_selectedObjectId = 0;
		m_status = "Ready";
	}

	ObjectRuleSet* WorldObjectManager::CreateRuleSet()
	{
		ObjectRuleSet ruleSet;
		ruleSet.id = m_nextRuleSetId++;
		ruleSet.name = "Rule Set " + std::to_string(ruleSet.id);
		if (m_selectedAssetId != 0)
		{
			ruleSet.assets.push_back({ m_selectedAssetId, 1.0f });
		}
		m_ruleSets.push_back(std::move(ruleSet));
		m_status = "Created object rule set";
		return &m_ruleSets.back();
	}

	ObjectRuleSet* WorldObjectManager::AddRuleSetRecord(const ObjectRuleSet& ruleSet)
	{
		m_ruleSets.push_back(ruleSet);
		m_nextRuleSetId = std::max(m_nextRuleSetId, ruleSet.id + 1);
		return &m_ruleSets.back();
	}

	ObjectRuleSet* WorldObjectManager::FindRuleSet(int ruleSetId)
	{
		auto it = std::find_if(m_ruleSets.begin(), m_ruleSets.end(), [&](const ObjectRuleSet& ruleSet) { return ruleSet.id == ruleSetId; });
		return it == m_ruleSets.end() ? nullptr : &(*it);
	}

	const ObjectRuleSet* WorldObjectManager::FindRuleSet(int ruleSetId) const
	{
		auto it = std::find_if(m_ruleSets.begin(), m_ruleSets.end(), [&](const ObjectRuleSet& ruleSet) { return ruleSet.id == ruleSetId; });
		return it == m_ruleSets.end() ? nullptr : &(*it);
	}

	ObjectExclusionZone* WorldObjectManager::CreateExclusionZone(int ruleSetId)
	{
		ObjectExclusionZone zone;
		zone.id = m_nextExclusionZoneId++;
		zone.name = "Exclusion Zone " + std::to_string(zone.id);
		zone.ruleSetId = ruleSetId;
		m_exclusionZones.push_back(std::move(zone));
		m_status = "Created exclusion zone";
		return &m_exclusionZones.back();
	}

	ObjectExclusionZone* WorldObjectManager::AddExclusionZoneRecord(const ObjectExclusionZone& zone)
	{
		m_exclusionZones.push_back(zone);
		m_nextExclusionZoneId = std::max(m_nextExclusionZoneId, zone.id + 1);
		return &m_exclusionZones.back();
	}

	ObjectExclusionZone* WorldObjectManager::FindExclusionZone(int zoneId)
	{
		auto it = std::find_if(m_exclusionZones.begin(), m_exclusionZones.end(), [&](const ObjectExclusionZone& zone) { return zone.id == zoneId; });
		return it == m_exclusionZones.end() ? nullptr : &(*it);
	}

	void WorldObjectManager::RemoveExclusionZone(int zoneId)
	{
		m_exclusionZones.erase(std::remove_if(m_exclusionZones.begin(), m_exclusionZones.end(), [&](const ObjectExclusionZone& zone)
		{
			return zone.id == zoneId;
		}), m_exclusionZones.end());
		m_status = "Removed exclusion zone";
	}
	int WorldObjectManager::ClearGenerated(int ruleSetId, int chunkX, int chunkZ)
	{
		auto matches = [&](const WorldObject& object)
		{
			bool matchesRule = ruleSetId == 0 || object.sourceRuleSetId == ruleSetId;
			bool matchesChunk = chunkX < 0 || chunkZ < 0 || (object.generatedChunkX == chunkX && object.generatedChunkZ == chunkZ);
			return object.generated && matchesRule && matchesChunk;
		};
		const WorldObject* selected = GetSelectedObject();
		bool clearSelection = selected && matches(*selected);
		auto before = m_objects.size();
		m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), matches), m_objects.end());
		if (clearSelection)
		{
			SelectObject(0);
		}
		return static_cast<int>(before - m_objects.size());
	}
	int WorldObjectManager::BakeGenerated(int ruleSetId)
	{
		int baked = 0;
		for (auto& object : m_objects)
		{
			if (object.generated && (ruleSetId == 0 || object.sourceRuleSetId == ruleSetId))
			{
				object.generated = false;
				object.sourceRuleSetId = 0;
				++baked;
			}
		}
		m_status = "Baked " + std::to_string(baked) + " generated objects";
		return baked;
	}

	void WorldObjectManager::RemoveRuleSet(int ruleSetId)
	{
		ClearGenerated(ruleSetId);
		m_exclusionZones.erase(std::remove_if(m_exclusionZones.begin(), m_exclusionZones.end(), [&](const ObjectExclusionZone& zone)
		{
			return zone.ruleSetId == ruleSetId;
		}), m_exclusionZones.end());
		m_ruleSets.erase(std::remove_if(m_ruleSets.begin(), m_ruleSets.end(), [&](const ObjectRuleSet& ruleSet)
		{
			return ruleSet.id == ruleSetId;
		}), m_ruleSets.end());
		m_status = "Removed object rule set";
	}

	int WorldObjectManager::GenerateRuleSet(int ruleSetId, std::shared_ptr<Terrain> terrain, int chunkX, int chunkZ)
	{
		ObjectRuleSet* ruleSet = FindRuleSet(ruleSetId);
		if (!ruleSet || !terrain || !ruleSet->enabled)
		{
			m_status = "Rule set is disabled or terrain is unavailable";
			return 0;
		}

		float totalWeight = 0.0f;
		for (const auto& entry : ruleSet->assets)
		{
			const ModelAsset* asset = FindAsset(entry.assetId);
			if (asset && asset->loaded && entry.weight > 0.0f)
			{
				totalWeight += entry.weight;
			}
		}
		if (totalWeight <= 0.0f)
		{
			m_status = "Rule set has no loaded assets with positive weight";
			return 0;
		}

		int worldSize = terrain->GetSize();
		float worldHalfSize = (static_cast<float>(worldSize) - 1.0f) * 0.5f;
		bool chunkOnly = chunkX >= 0 && chunkZ >= 0;
		int chunkCount = terrain->GetChunkManager()->GetChunkCount();
		int chunkStride = std::max(terrain->GetChunkManager()->GetChunkSize() - 1, 1);
		if (chunkOnly && (chunkX >= chunkCount || chunkZ >= chunkCount))
		{
			m_status = "Chunk coordinates are outside the terrain";
			return 0;
		}
		float generationMinX = -worldHalfSize;
		float generationMaxX = worldHalfSize;
		float generationMinZ = -worldHalfSize;
		float generationMaxZ = worldHalfSize;
		if (chunkOnly)
		{
			generationMinX = chunkX * static_cast<float>(chunkStride) - worldHalfSize;
			generationMinZ = chunkZ * static_cast<float>(chunkStride) - worldHalfSize;
			generationMaxX = std::min(generationMinX + static_cast<float>(chunkStride), worldHalfSize);
			generationMaxZ = std::min(generationMinZ + static_cast<float>(chunkStride), worldHalfSize);
		}
		auto& terrainMasks = terrain->GetChunkManager()->GetMasks();
		std::vector<float> placementMask;
		if (!ruleSet->maskName.empty())
		{
			auto mask = terrainMasks.find(ruleSet->maskName);
			if (mask == terrainMasks.end() || !mask->second || !mask->second->texture)
			{
				m_status = "Placement mask not found: " + ruleSet->maskName;
				return 0;
			}
			placementMask = mask->second->texture->ReadFloatData();
			size_t expectedSize = static_cast<size_t>(worldSize) * static_cast<size_t>(worldSize);
			if (placementMask.size() != expectedSize)
			{
				m_status = "Placement mask must be a scalar terrain mask: " + ruleSet->maskName;
				return 0;
			}
		}

		std::vector<float> waterMask;
		std::vector<float> riverMask;
		auto readAvoidanceMask = [&](const char* name, bool required, std::vector<float>& output)
		{
			if (!required)
			{
				return true;
			}
			auto mask = terrainMasks.find(name);
			if (mask == terrainMasks.end() || !mask->second || !mask->second->texture)
			{
				m_status = std::string("Avoidance mask not found: ") + name;
				return false;
			}
			output = mask->second->texture->ReadFloatData();
			if (output.size() != static_cast<size_t>(worldSize) * static_cast<size_t>(worldSize))
			{
				m_status = std::string("Avoidance mask must be scalar: ") + name;
				return false;
			}
			return true;
		};
		if (!readAvoidanceMask("WaterMask", ruleSet->avoidWaterDistance > 0.0f, waterMask) ||
			!readAvoidanceMask("RiverChannelMask", ruleSet->avoidRiverDistance > 0.0f, riverMask))
		{
			return 0;
		}

		auto sampleScalarMask = [&](const std::vector<float>& data, float worldX, float worldZ)
		{
			float textureX = std::clamp(worldX + worldHalfSize, 0.0f, static_cast<float>(worldSize - 1));
			float textureZ = std::clamp(worldZ + worldHalfSize, 0.0f, static_cast<float>(worldSize - 1));
			int x0 = static_cast<int>(std::floor(textureX));
			int z0 = static_cast<int>(std::floor(textureZ));
			int x1 = std::min(x0 + 1, worldSize - 1);
			int z1 = std::min(z0 + 1, worldSize - 1);
			float tx = textureX - static_cast<float>(x0);
			float tz = textureZ - static_cast<float>(z0);
			auto valueAt = [&](int px, int pz) { return data[static_cast<size_t>(pz) * worldSize + px]; };
			float row0 = glm::mix(valueAt(x0, z0), valueAt(x1, z0), tx);
			float row1 = glm::mix(valueAt(x0, z1), valueAt(x1, z1), tx);
			return glm::mix(row0, row1, tz);
		};

		auto isNearMaskedArea = [&](const std::vector<float>& data, float worldX, float worldZ, float distance, float threshold)
		{
			if (data.empty() || distance <= 0.0f)
			{
				return false;
			}
			float step = std::max(1.0f, distance / 8.0f);
			float distanceSquared = distance * distance;
			for (float dz = -distance; dz <= distance; dz += step)
			{
				for (float dx = -distance; dx <= distance; dx += step)
				{
					if (dx * dx + dz * dz <= distanceSquared && sampleScalarMask(data, worldX + dx, worldZ + dz) >= threshold)
					{
						return true;
					}
				}
			}
			return false;
		};

		auto isExcluded = [&](float worldX, float worldZ)
		{
			for (const auto& zone : m_exclusionZones)
			{
				if (!zone.enabled || (zone.ruleSetId != 0 && zone.ruleSetId != ruleSetId))
				{
					continue;
				}
				glm::vec2 delta = glm::vec2(worldX, worldZ) - zone.center;
				if (zone.shape == ObjectExclusionShape::Circle)
				{
					float radius = std::max(zone.radius, 0.0f);
					if (glm::dot(delta, delta) <= radius * radius)
					{
						return true;
					}
				}
				else
				{
					glm::vec2 extents = glm::abs(zone.halfExtents);
					if (std::abs(delta.x) <= extents.x && std::abs(delta.y) <= extents.y)
					{
						return true;
					}
				}
			}
			return false;
		};
		ClearGenerated(ruleSetId, chunkX, chunkZ);
		std::mt19937::result_type randomSeed = static_cast<std::mt19937::result_type>(ruleSet->seed);
		if (chunkOnly)
		{
			randomSeed ^= static_cast<std::mt19937::result_type>(chunkX * 73856093) ^ static_cast<std::mt19937::result_type>(chunkZ * 19349663);
		}
		std::mt19937 random(randomSeed);
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		float spacing = std::max(ruleSet->spacing, 4.0f);
		float density = std::clamp(ruleSet->density, 0.0f, 1.0f);
		float minScale = std::min(ruleSet->minScale, ruleSet->maxScale);
		float maxScale = std::max(ruleSet->minScale, ruleSet->maxScale);
		constexpr int maxGeneratedObjects = 50000;
		int generatedCount = 0;
		std::unordered_map<long long, std::vector<glm::vec2>> acceptedCells;
		auto spatialKey = [](int cellX, int cellZ)
		{
			return (static_cast<long long>(cellX) << 32) | static_cast<unsigned int>(cellZ);
		};
		auto isTooClose = [&](float worldX, float worldZ)
		{
			int candidateCellX = static_cast<int>(std::floor((worldX + worldHalfSize) / spacing));
			int candidateCellZ = static_cast<int>(std::floor((worldZ + worldHalfSize) / spacing));
			float spacingSquared = spacing * spacing;
			for (int dz = -1; dz <= 1; ++dz)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					auto found = acceptedCells.find(spatialKey(candidateCellX + dx, candidateCellZ + dz));
					if (found == acceptedCells.end())
					{
						continue;
					}
					for (const glm::vec2& position : found->second)
					{
						glm::vec2 delta = position - glm::vec2(worldX, worldZ);
						if (glm::dot(delta, delta) < spacingSquared)
						{
							return true;
						}
					}
				}
			}
			return false;
		};

		for (const auto& existing : m_objects)
		{
			if (!existing.generated || existing.sourceRuleSetId != ruleSetId)
			{
				continue;
			}
			int existingCellX = static_cast<int>(std::floor((existing.transform.position.x + worldHalfSize) / spacing));
			int existingCellZ = static_cast<int>(std::floor((existing.transform.position.z + worldHalfSize) / spacing));
			acceptedCells[spatialKey(existingCellX, existingCellZ)].push_back(glm::vec2(existing.transform.position.x, existing.transform.position.z));
		}

		for (float cellZ = generationMinZ; cellZ <= generationMaxZ && generatedCount < maxGeneratedObjects; cellZ += spacing)
		{
			for (float cellX = generationMinX; cellX <= generationMaxX && generatedCount < maxGeneratedObjects; cellX += spacing)
			{
				if (unit(random) > density)
				{
					continue;
				}
				float jitter = spacing * 0.38f;
				float x = std::clamp(cellX + (unit(random) * 2.0f - 1.0f) * jitter, -worldHalfSize, worldHalfSize);
				float z = std::clamp(cellZ + (unit(random) * 2.0f - 1.0f) * jitter, -worldHalfSize, worldHalfSize);
				if (isExcluded(x, z) || isTooClose(x, z))
				{
					continue;
				}
				float height = terrain->GetTerrainHeightAt(x, z);
				if (height < ruleSet->minHeight || height > ruleSet->maxHeight)
				{
					continue;
				}
				if (!placementMask.empty())
				{
					float maskValue = sampleScalarMask(placementMask, x, z);
					float maskMin = std::min(ruleSet->minMaskValue, ruleSet->maxMaskValue);
					float maskMax = std::max(ruleSet->minMaskValue, ruleSet->maxMaskValue);
					if (maskValue < maskMin || maskValue > maskMax)
					{
						continue;
					}
				}
				if (isNearMaskedArea(waterMask, x, z, ruleSet->avoidWaterDistance, ruleSet->waterThreshold) ||
					isNearMaskedArea(riverMask, x, z, ruleSet->avoidRiverDistance, ruleSet->riverThreshold))
				{
					continue;
				}

				float left = terrain->GetTerrainHeightAt(x - 1.0f, z);
				float right = terrain->GetTerrainHeightAt(x + 1.0f, z);
				float down = terrain->GetTerrainHeightAt(x, z - 1.0f);
				float up = terrain->GetTerrainHeightAt(x, z + 1.0f);
				glm::vec3 normal = glm::normalize(glm::vec3(left - right, 2.0f, down - up));
				float slopeDegrees = glm::degrees(std::acos(std::clamp(normal.y, -1.0f, 1.0f)));
				if (slopeDegrees < ruleSet->minSlopeDegrees || slopeDegrees > ruleSet->maxSlopeDegrees)
				{
					continue;
				}

				float choice = unit(random) * totalWeight;
				const ModelAsset* selectedAsset = nullptr;
				for (const auto& entry : ruleSet->assets)
				{
					const ModelAsset* candidate = FindAsset(entry.assetId);
					if (!candidate || !candidate->loaded || entry.weight <= 0.0f)
					{
						continue;
					}
					choice -= entry.weight;
					selectedAsset = candidate;
					if (choice <= 0.0f)
					{
						break;
					}
				}
				if (!selectedAsset)
				{
					continue;
				}

				WorldObject object;
				object.assetId = selectedAsset->id;
				object.transform.position = { x, height, z };
				float scale = minScale + (maxScale - minScale) * unit(random);
				object.transform.scale = selectedAsset->defaultScale * scale;
				auto randomRotation = [&](float minimum, float maximum)
				{
					float low = std::min(minimum, maximum);
					float high = std::max(minimum, maximum);
					return glm::radians(low + (high - low) * unit(random));
				};
				object.transform.rotation.x = randomRotation(ruleSet->minRotationDegrees.x, ruleSet->maxRotationDegrees.x);
				object.transform.rotation.y = randomRotation(ruleSet->minRotationDegrees.y, ruleSet->maxRotationDegrees.y);
				object.transform.rotation.z = randomRotation(ruleSet->minRotationDegrees.z, ruleSet->maxRotationDegrees.z);
				if (ruleSet->alignToNormal)
				{
					object.transform.rotation.x += std::atan2(normal.z, normal.y);
					object.transform.rotation.z -= std::atan2(normal.x, normal.y);
				}

				glm::mat4 candidateModel = BuildModelMatrix(object, *selectedAsset);
				glm::vec3 transformedMin(std::numeric_limits<float>::max());
				glm::vec3 transformedMax(std::numeric_limits<float>::lowest());
				for (int corner = 0; corner < 8; ++corner)
				{
					glm::vec3 local(
						(corner & 1) ? selectedAsset->boundsMax.x : selectedAsset->boundsMin.x,
						(corner & 2) ? selectedAsset->boundsMax.y : selectedAsset->boundsMin.y,
						(corner & 4) ? selectedAsset->boundsMax.z : selectedAsset->boundsMin.z);
					glm::vec3 worldCorner = glm::vec3(candidateModel * glm::vec4(local, 1.0f));
					transformedMin = glm::min(transformedMin, worldCorner);
					transformedMax = glm::max(transformedMax, worldCorner);
				}
				if (transformedMin.x < -worldHalfSize || transformedMax.x > worldHalfSize ||
					transformedMin.z < -worldHalfSize || transformedMax.z > worldHalfSize)
				{
					continue;
				}
				GroundObjectToTerrain(object, terrain);

				object.id = ReserveObjectId();
				object.name = selectedAsset->name + " [" + ruleSet->name + "] " + std::to_string(object.id);
				object.snapToTerrain = true;
				object.alignToNormal = ruleSet->alignToNormal;
				object.generated = true;
				object.sourceRuleSetId = ruleSet->id;
				object.generatedChunkX = std::clamp(static_cast<int>(std::floor((x + worldHalfSize) / static_cast<float>(chunkStride))), 0, chunkCount - 1);
				object.generatedChunkZ = std::clamp(static_cast<int>(std::floor((z + worldHalfSize) / static_cast<float>(chunkStride))), 0, chunkCount - 1);
				int acceptedCellX = static_cast<int>(std::floor((x + worldHalfSize) / spacing));
				int acceptedCellZ = static_cast<int>(std::floor((z + worldHalfSize) / spacing));
				acceptedCells[spatialKey(acceptedCellX, acceptedCellZ)].push_back(glm::vec2(x, z));
				m_objects.push_back(std::move(object));
				++generatedCount;
			}
		}

		m_status = "Generated " + std::to_string(generatedCount) + " objects from " + ruleSet->name;
		return generatedCount;
	}
	void WorldObjectManager::GroundObjectToTerrain(WorldObject& object, std::shared_ptr<Terrain> terrain)
	{
		const ModelAsset* asset = FindAsset(object.assetId);
		if (!asset || !terrain)
		{
			return;
		}

		glm::mat4 model = BuildModelMatrix(object, *asset);
		float correction = std::numeric_limits<float>::lowest();
		for (const glm::vec3& localPoint : asset->groundingPoints)
		{
			glm::vec3 worldPoint = glm::vec3(model * glm::vec4(localPoint, 1.0f));
			float terrainHeight = terrain->GetTerrainHeightAt(worldPoint.x, worldPoint.z);
			correction = std::max(correction, terrainHeight - worldPoint.y);
		}
		if (asset->groundingPoints.empty())
		{
			for (int corner = 0; corner < 8; ++corner)
			{
				glm::vec3 localPoint((corner & 1) ? asset->boundsMax.x : asset->boundsMin.x,
					(corner & 2) ? asset->boundsMax.y : asset->boundsMin.y,
					(corner & 4) ? asset->boundsMax.z : asset->boundsMin.z);
				glm::vec3 worldPoint = glm::vec3(model * glm::vec4(localPoint, 1.0f));
				correction = std::max(correction, terrain->GetTerrainHeightAt(worldPoint.x, worldPoint.z) - worldPoint.y);
			}
		}

		if (correction != std::numeric_limits<float>::lowest())
		{
			object.transform.position.y += correction;
		}
		object.terrainOffset = object.transform.position.y - terrain->GetTerrainHeightAt(object.transform.position.x, object.transform.position.z);
	}

	void WorldObjectManager::UpdateTerrainSnap(std::shared_ptr<Terrain> terrain)
	{
		if (!terrain)
		{
			return;
		}
		for (auto& object : m_objects)
		{
			if (!object.visible || !object.snapToTerrain || object.freeTransform)
			{
				continue;
			}
			object.transform.position.y = terrain->GetTerrainHeightAt(object.transform.position.x, object.transform.position.z) + object.terrainOffset;
		}
	}

	void WorldObjectManager::Draw(Renderer& renderer)
	{
		if (!m_objectMaterial)
		{
			return;
		}

		constexpr float objectRenderDistance = 5000.0f;
		std::unordered_map<int, std::vector<glm::mat4>> batches;
		for (const auto& object : m_objects)
		{
			if (!object.visible)
			{
				continue;
			}
			const ModelAsset* asset = FindAsset(object.assetId);
			if (!asset || !asset->loaded)
			{
				continue;
			}

			glm::mat4 model = BuildModelMatrix(object, *asset);
			glm::vec3 boundsCenter = glm::vec3(model * glm::vec4(asset->boundsCenter, 1.0f));
			float boundsScale = std::max({
				glm::length(glm::vec3(model[0])),
				glm::length(glm::vec3(model[1])),
				glm::length(glm::vec3(model[2])) });
			float boundsRadius = asset->boundsRadius * boundsScale;
			if (!renderer.IsSphereVisible(boundsCenter, boundsRadius, objectRenderDistance))
			{
				continue;
			}

			if (object.selected)
			{
				DrawObject(renderer, object, *asset, glm::vec3(1.0f, 0.88f, 0.45f));
			}
			else
			{
				batches[asset->id].push_back(model);
			}
		}

		for (const auto& [assetId, transforms] : batches)
		{
			const ModelAsset* asset = FindAsset(assetId);
			if (!asset || transforms.empty())
			{
				continue;
			}

			m_objectMaterial->SetUniformVec3("u_tint", glm::vec3(1.0f));
			if (!asset->submeshes.empty())
			{
				for (const auto& submesh : asset->submeshes)
				{
					if (!submesh.mesh)
					{
						continue;
					}
					m_objectMaterial->SetUniformInt("u_hasAlbedo", submesh.albedoTexture ? 1 : 0);
					if (submesh.albedoTexture)
					{
						m_objectMaterial->SetUniformTexture("u_albedo", submesh.albedoTexture);
					}
					renderer.DrawMeshInstanced(submesh.mesh, m_objectMaterial, transforms);
				}
			}
			else if (asset->mesh)
			{
				m_objectMaterial->SetUniformInt("u_hasAlbedo", asset->albedoTexture ? 1 : 0);
				if (asset->albedoTexture)
				{
					m_objectMaterial->SetUniformTexture("u_albedo", asset->albedoTexture);
				}
				renderer.DrawMeshInstanced(asset->mesh, m_objectMaterial, transforms);
			}
		}
	}
	void WorldObjectManager::DrawPreview(Renderer& renderer, int assetId, const Transform& transform, bool validPlacement)
	{
		if (!m_objectMaterial)
		{
			return;
		}

		const ModelAsset* asset = FindAsset(assetId);
		if (!asset || !asset->loaded)
		{
			return;
		}

		WorldObject preview;
		preview.id = -1;
		preview.assetId = assetId;
		preview.transform = transform;
		preview.visible = true;
		glm::vec3 tint = validPlacement ? glm::vec3(0.45f, 0.9f, 1.0f) : glm::vec3(1.0f, 0.35f, 0.25f);
		DrawObject(renderer, preview, *asset, tint);
	}

	void WorldObjectManager::DrawAssetPreview(Renderer& renderer, const ModelAsset& asset)
	{
		if (!m_objectMaterial || !asset.loaded)
		{
			return;
		}
		WorldObject preview;
		preview.id = -1;
		preview.assetId = asset.id;
		preview.transform.scale = asset.defaultScale;
		DrawObject(renderer, preview, asset, glm::vec3(0.55f, 0.85f, 1.0f));
	}

	void WorldObjectManager::RefreshAssetInstances(int assetId, std::shared_ptr<Terrain> terrain)
	{
		if (ModelAsset* asset = FindAsset(assetId))
		{
			UpdateGroundingPoints(*asset);
		}
		for (auto& object : m_objects)
		{
			if (object.assetId == assetId && object.snapToTerrain && !object.freeTransform)
			{
				GroundObjectToTerrain(object, terrain);
			}
		}
		m_status = "Updated asset instances";
	}
	void WorldObjectManager::DrawObject(Renderer& renderer, const WorldObject& object, const ModelAsset& asset, const glm::vec3& tint)
	{
		glm::mat4 modelMatrix = BuildModelMatrix(object, asset);
		if (!asset.submeshes.empty())
		{
			for (const auto& submesh : asset.submeshes)
			{
				if (!submesh.mesh)
				{
					continue;
				}
				m_objectMaterial->SetUniformVec3("u_tint", tint);
				m_objectMaterial->SetUniformInt("u_hasAlbedo", submesh.albedoTexture ? 1 : 0);
				if (submesh.albedoTexture)
				{
					m_objectMaterial->SetUniformTexture("u_albedo", submesh.albedoTexture);
				}
				renderer.DrawMesh(submesh.mesh, m_objectMaterial, modelMatrix);
			}
			return;
		}

		if (!asset.mesh)
		{
			return;
		}
		m_objectMaterial->SetUniformVec3("u_tint", tint);
		m_objectMaterial->SetUniformInt("u_hasAlbedo", asset.albedoTexture ? 1 : 0);
		if (asset.albedoTexture)
		{
			m_objectMaterial->SetUniformTexture("u_albedo", asset.albedoTexture);
		}
		renderer.DrawMesh(asset.mesh, m_objectMaterial, modelMatrix);
	}

	ModelAsset* WorldObjectManager::FindAsset(int assetId)
	{
		auto it = std::find_if(m_assets.begin(), m_assets.end(), [&](const ModelAsset& asset) { return asset.id == assetId; });
		return it == m_assets.end() ? nullptr : &(*it);
	}

	const ModelAsset* WorldObjectManager::FindAsset(int assetId) const
	{
		auto it = std::find_if(m_assets.begin(), m_assets.end(), [&](const ModelAsset& asset) { return asset.id == assetId; });
		return it == m_assets.end() ? nullptr : &(*it);
	}

	WorldObject* WorldObjectManager::FindObject(int objectId)
	{
		auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const WorldObject& object) { return object.id == objectId; });
		return it == m_objects.end() ? nullptr : &(*it);
	}

	const WorldObject* WorldObjectManager::FindObject(int objectId) const
	{
		auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const WorldObject& object) { return object.id == objectId; });
		return it == m_objects.end() ? nullptr : &(*it);
	}

	WorldObject* WorldObjectManager::GetSelectedObject()
	{
		return FindObject(m_selectedObjectId);
	}

	const WorldObject* WorldObjectManager::GetSelectedObject() const
	{
		return FindObject(m_selectedObjectId);
	}

	void WorldObjectManager::SelectObject(int objectId)
	{
		m_selectedObjectId = objectId;
		for (auto& object : m_objects)
		{
			object.selected = object.id == objectId;
		}
	}

	void WorldObjectManager::SelectAsset(int assetId)
	{
		m_selectedAssetId = assetId;
	}

	void WorldObjectManager::MarkCleanSelection()
	{
		for (auto& object : m_objects)
		{
			object.selected = object.id == m_selectedObjectId;
		}
	}

	glm::mat4 WorldObjectManager::BuildModelMatrix(const WorldObject& object, const ModelAsset& asset) const
	{
		glm::mat4 model{ 1.0f };
		model = glm::translate(model, object.transform.position);
		model = glm::rotate(model, object.transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, object.transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, object.transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, asset.pivotOffset);
		model = glm::rotate(model, asset.importRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, asset.importRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, asset.importRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, object.transform.scale);
		return model;
	}
}