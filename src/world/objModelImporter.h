#ifndef SRC_WORLD_OBJ_MODEL_IMPORTER_H_
#define SRC_WORLD_OBJ_MODEL_IMPORTER_H_

#include "world/modelAsset.h"

#include <string>
#include <vector>

namespace pwg
{
	struct ObjImportResult
	{
		std::vector<ModelSubmesh> submeshes;
		std::shared_ptr<Mesh> mesh;
		std::string message;
		glm::vec3 diffuseColor{ 0.78f, 0.78f, 0.74f };
		std::string albedoPath;
		bool success{ false };
	};

	class ObjModelImporter
	{
	public:
		static ObjImportResult Load(const std::string& path);
	};
}

#endif // !SRC_WORLD_OBJ_MODEL_IMPORTER_H_