#ifndef SRC_GRAPHICS_MESH_MESH_MANAGER_H
#define SRC_GRAPHICS_MESH_MESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "mesh.h"

namespace pwg
{
	/**
	* @brief Class responsible for managing and storing mesh instances.
	* Provides centralized creation and access to all meshes used by the renderer.
	*/
	class MeshManager
	{
	public:

		/**
		* @brief Default constructor.
		* Initializes an empty mesh manager.
		*/
		MeshManager();

		/**
		* @brief Destructor.
		* Releases all stored mesh resources.
		*/
		~MeshManager();

		/**
		* @brief Creates a plane mesh and stores it in the manager.
		* @param size Number of vertices defining the plane resolution.
		* @param name Unique identifier for the mesh.
		* @return Identifier of the created mesh.
		*/
		std::string CreatePlaneMesh(int size, std::string name);

		/**
		* @brief Creates a plane mesh and returns it.
		* @param size Number of vertices defining the plane resolution.
		* @return Shared pointer of the created mesh.
		*/
		static std::shared_ptr<Mesh> CreatePlaneMesh(int size, int lod = 0);

		/**
		* @brief Creates a sphere mesh and stores it in the manager.
		* @param radius Radius of the sphere.
		* @param sectorCount Number of longitudinal segments.
		* @param sliceCount Number of latitudinal segments.
		* @param name Unique identifier for the mesh.
		* @return Identifier of the created mesh.
		*/
		std::string CreateSphereMesh(float radius, int sectorCount, int sliceCount, std::string name);

		/**
		* @brief Retrieves a mesh by its identifier.
		* @param meshID Name or identifier of the mesh.
		* @return Shared pointer to the mesh instance, or nullptr if not found.
		*/
		std::shared_ptr<Mesh> GetMesh(std::string meshID);

	private:
		std::string m_meshID;												/**< Identifier of the most recently created mesh */
		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;	/**< Container storing all managed meshes */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_MESH_MESH_MANAGER_H
