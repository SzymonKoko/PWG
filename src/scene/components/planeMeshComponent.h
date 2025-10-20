#ifndef SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
#define SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H

#include <string>

namespace pwg::components
{
	/**
	 * @brief Component representing a plane mesh.
	 * Stores the size of the plane and a flag indicating if the mesh has been changed.
	 */
	struct PlaneMeshComponent
	{
		static constexpr const char* Name = "Plane Mesh Component"; /**< Component name identifier. */

		int size;				/**< Size of the plane mesh (number of vertices per side). */
		bool changed = true;	/**< Flag indicating whether the plane mesh has been modified. */

		/**
		 * @brief Constructor that sets the size of the plane mesh.
		 * @param size Number of vertices per side of the plane.
		 */
		PlaneMeshComponent(int size) : size(size) {}
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_PLANE_MESH_COMPONENT_H
