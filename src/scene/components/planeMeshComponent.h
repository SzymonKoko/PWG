#ifndef SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
#define SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H

#include <string>

namespace pwg::components
{
	struct PlaneMeshComponent
	{
		int width;
		int height;
		bool changed = true;

		PlaneMeshComponent(int width, int height) : width(width), height(height) {}
	};
}
#endif // !SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
