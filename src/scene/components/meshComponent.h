#ifndef SRC_SCENE_COMPONENTS_MESH_COMPONENT_H
#define SRC_SCENE_COMPONENTS_MESH_COMPONENT_H

#include <string>

namespace pwg::components
{
	/**
	 * @brief Component that stores the ID of a mesh associated with an entity.
	 * Used to link an entity to a mesh in the MeshManager.
	 */
	struct MeshComponent
	{
		static constexpr const char* Name = "Mesh Component"; /**< Component name identifier. */

		std::string meshID = ""; /**< ID of the mesh used by this entity. */

		/**
		 * @brief Default constructor.
		 */
		MeshComponent() = default;
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_MESH_COMPONENT_H
