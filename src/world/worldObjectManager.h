#ifndef SRC_WORLD_WORLD_OBJECT_MANAGER_H_
#define SRC_WORLD_WORLD_OBJECT_MANAGER_H_

#include "graphics/material/material.h"
#include "graphics/renderer/irenderable.h"
#include "scene/cameras/icamera.h"
#include "terrain/terrain.h"
#include "world/modelAsset.h"

#include <functional>
#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <vector>

namespace pwg
{
	class WorldObjectManager
	{
	public:
		explicit WorldObjectManager(std::shared_ptr<Material> objectMaterial);

		ModelAsset* ImportObj(const std::string& path);
		ModelAsset* AddAssetRecord(const ModelAsset& asset);
		WorldObject* AddObjectRecord(const WorldObject& object);
		WorldObject* CreateObject(int assetId, const glm::vec3& position);
		WorldObject* DuplicateSelected();
		void DeleteSelected();
		void Clear();
		void UpdateTerrainSnap(std::shared_ptr<Terrain> terrain);
		void Draw(Renderer& renderer);
		void DrawPreview(Renderer& renderer, int assetId, const Transform& transform, bool validPlacement);

		ModelAsset* FindAsset(int assetId);
		const ModelAsset* FindAsset(int assetId) const;
		WorldObject* FindObject(int objectId);
		const WorldObject* FindObject(int objectId) const;
		WorldObject* GetSelectedObject();
		const WorldObject* GetSelectedObject() const;
		void SelectObject(int objectId);
		void SelectAsset(int assetId);
		void MarkCleanSelection();

		std::vector<ModelAsset>& GetAssets() { return m_assets; }
		const std::vector<ModelAsset>& GetAssets() const { return m_assets; }
		std::vector<WorldObject>& GetObjects() { return m_objects; }
		const std::vector<WorldObject>& GetObjects() const { return m_objects; }
		int GetSelectedAssetId() const { return m_selectedAssetId; }
		const std::string& GetStatus() const { return m_status; }
		void SetStatus(const std::string& status) { m_status = status; }
		int ReserveAssetId();
		int ReserveObjectId();

	private:
		glm::mat4 BuildModelMatrix(const WorldObject& object, const ModelAsset& asset) const;
		void DrawObject(Renderer& renderer, const WorldObject& object, const ModelAsset& asset, const glm::vec3& tint);

		std::vector<ModelAsset> m_assets;
		std::vector<WorldObject> m_objects;
		std::shared_ptr<Material> m_objectMaterial;
		int m_nextAssetId{ 1 };
		int m_nextObjectId{ 1 };
		int m_selectedAssetId{ 0 };
		int m_selectedObjectId{ 0 };
		std::string m_status{ "Ready" };
	};
}

#endif // !SRC_WORLD_WORLD_OBJECT_MANAGER_H_


