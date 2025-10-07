#ifndef SRC_GRAPHICS_MESH_MESH_MANAGER_H
#define SRC_GRAPHICS_MESH_MESH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "mesh.h"

namespace pwg
{
	class MeshManager
	{
	public:
		MeshManager();
		~MeshManager();

		std::string CreateMesh(int width, int height, std::string name);

		std::shared_ptr<Mesh> GetMesh(std::string meshID);
	private:
		std::string m_meshID;
		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
	};
}
#endif // !SRC_GRAPHICS_MESH_MESH_MANAGER_H
