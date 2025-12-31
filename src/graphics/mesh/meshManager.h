#ifndef SRC_GRAPHICS_MESH_MESH_MANAGER_H
#define SRC_GRAPHICS_MESH_MESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "mesh.h"

namespace pwg
{
	/**
	* @brief Class responsible for managing and storing all mesh instances.
	* Provides functionality to create, retrieve and keep track of loaded meshes.
	*/
	class MeshManager
	{
	public:

		/**
		* @brief Default constructor. Initializes an empty mesh manager.
		*/
		MeshManager();

		/**
		* @brief Destructor. Cleans up stored meshes.
		*/
		~MeshManager();

		/**
		* @brief Creates a new mesh and stores it in the manager.
		* @param size Number of vertices for the new mesh.
		* @param name Unique name (ID) for the mesh.
		* @return Returns the name (ID) of the created mesh.
		*/
		std::string CreatePlaneMesh(int size, std::string name);

		std::string CreateSphereMesh(float radius, int sectorCount, int sliceCount, std::string name);

		/**
		* @brief Retrieves a mesh from the manager by its ID.
		* @param meshID Name or identifier of the mesh.
		* @return Shared pointer to the requested mesh instance.
		*/
		std::shared_ptr<Mesh> GetMesh(std::string meshID);

	private:
		std::string m_meshID;												/**< Identifier of the most recently created mesh. */
		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;	/**< Container storing all mesh instances. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_MESH_MESH_MANAGER_H
