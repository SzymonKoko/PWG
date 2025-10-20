#ifndef SRC_SCENE_COMPONENTS_TAG_COMPONENT
#define SRC_SCENE_COMPONENTS_TAG_COMPONENT

#include <string>

namespace pwg::components
{
	struct TagComponent
	{
		static constexpr const char* Name = "Tag Component";

		std::string name;

		TagComponent(const std::string& name = "") : name(name) {}
	};
}
#endif // !SRC_SCENE_COMPONENTS_TAG_COMPONENT
