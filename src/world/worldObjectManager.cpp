#include "worldObjectManager.h"

#include "world/objModelImporter.h"

#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

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
				asset.boundsRadius = 1.0f;
				asset.defaultScale = glm::vec3(1.0f);
				asset.pivotOffset = glm::vec3(0.0f);
				return;
			}

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
		m_nextAssetId = 1;
		m_nextObjectId = 1;
		m_selectedAssetId = 0;
		m_selectedObjectId = 0;
		m_status = "Ready";
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
			object.transform.position.y = terrain->GetTerrainHeightAt(object.transform.position.x, object.transform.position.z) - 4.0f;
		}
	}

	void WorldObjectManager::Draw(Renderer& renderer)
	{
		if (!m_objectMaterial)
		{
			return;
		}

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

			glm::vec3 tint = object.selected ? glm::vec3(1.0f, 0.88f, 0.45f) : glm::vec3(1.0f);
			DrawObject(renderer, object, *asset, tint);
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
		model = glm::translate(model, object.transform.position + asset.pivotOffset);
		model = glm::rotate(model, object.transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, object.transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, object.transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, object.transform.scale);
		return model;
	}
}