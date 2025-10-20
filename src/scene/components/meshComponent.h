#ifndef SRC_SCENE_COMPONENTS_MESH_COMPONENT_H
#define SRC_SCENE_COMPONENTS_MESH_COMPONENT_H

#include <string>

namespace pwg::components
{
	struct MeshComponent
	{
		static constexpr const char* Name = "Mesh Component";

		std::string meshID = "";

		MeshComponent() = default;
	};
}
#endif // !SRC_SCENE_COMPONENTS_MESH_COMPONENT_H
