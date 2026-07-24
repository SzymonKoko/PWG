#ifndef SRC_WORLD_MODEL_ASSET_H_
#define SRC_WORLD_MODEL_ASSET_H_

#include "graphics/common/transform.h"
#include "graphics/mesh/mesh.h"
#include "graphics/texture/texture.h"

#include <memory>
#include <string>
#include <vector>

namespace pwg
{
	enum class ObjectSurfaceMode
	{
		SnapToTerrain,
		FreeTransform,
		AlignToNormal
	};

	struct ModelSubmesh
	{
		std::string name;
		std::shared_ptr<Mesh> mesh;
		glm::vec3 diffuseColor{ 0.78f, 0.78f, 0.74f };
		std::string albedoPath;
		std::shared_ptr<Texture> albedoTexture;
	};

	struct ModelAsset
	{
		int id{ 0 };
		std::string name;
		std::string path;
		glm::vec3 defaultScale{ 1.0f, 1.0f, 1.0f };
		glm::vec3 pivotOffset{ 0.0f, 0.0f, 0.0f };
		glm::vec3 diffuseColor{ 0.78f, 0.78f, 0.74f };
		std::string albedoPath;
		std::shared_ptr<Texture> albedoTexture;
		glm::vec3 boundsCenter{ 0.0f, 0.0f, 0.0f };
		float boundsRadius{ 1.0f };
		std::vector<ModelSubmesh> submeshes;
		ObjectSurfaceMode defaultSurfaceMode{ ObjectSurfaceMode::SnapToTerrain };
		std::shared_ptr<Mesh> mesh;
		bool loaded{ false };
		std::string status;
	};

	struct WorldObject
	{
		int id{ 0 };
		int assetId{ 0 };
		std::string name;
		Transform transform;
		bool snapToTerrain{ true };
		bool freeTransform{ false };
		bool alignToNormal{ false };
		bool visible{ true };
		bool selected{ false };
	};
}

#endif // !SRC_WORLD_MODEL_ASSET_H_
