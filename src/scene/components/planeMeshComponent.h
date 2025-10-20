#ifndef SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
#define SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H

#include <string>

namespace pwg::components
{
	struct PlaneMeshComponent
	{
		static constexpr const char* Name = "Plane Mesh Component";

		int size;
		bool changed = true;

		PlaneMeshComponent(int size) : size(size) {}
	};
}
#endif // !SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
