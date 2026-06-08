#ifndef SRC_PROJECT_PROJECT_SERIALIZER_H_
#define SRC_PROJECT_PROJECT_SERIALIZER_H_

#include <string>

namespace pwg
{
	class EditorScene;

	class ProjectSerializer
	{
	public:
		static bool Save(EditorScene& scene, const std::string& path);
		static bool Load(EditorScene& scene, const std::string& path);
	};
}

#endif // !SRC_PROJECT_PROJECT_SERIALIZER_H_
