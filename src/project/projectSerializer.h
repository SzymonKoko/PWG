#ifndef SRC_PROJECT_PROJECT_SERIALIZER_H_
#define SRC_PROJECT_PROJECT_SERIALIZER_H_

#include <string>

namespace pwg
{
	class EditorScene;
	struct TerrainGraphAsset;

	class ProjectSerializer
	{
	public:
		static bool Save(EditorScene& scene, const std::string& path);
		static bool Load(EditorScene& scene, const std::string& path);

		/**
		* @brief Reads only the universal terrain graph (`terrainGraphV2`) of a project file into `asset`, with the same
		* reader and migrations as Load. Used by the experiment runner's `graph.file`. Returns false and leaves `asset`
		* unchanged if the file is not a PWG project or has no graph.
		*/
		static bool LoadGraph(const std::string& path, TerrainGraphAsset& asset);
	};
}

#endif // !SRC_PROJECT_PROJECT_SERIALIZER_H_
