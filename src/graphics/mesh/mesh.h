#ifndef SRC_GRAPHICS_MESH_MESH_H
#define SRC_GRAPHICS_MESH_MESH_H

#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace pwg
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	class Mesh 
	{
	public:
		Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		void Update(unsigned int shaderID);
		void Draw();
	private:
		std::vector<Vertex> m_vertices;
		std::vector<unsigned int> m_indices;

		unsigned int m_vaoID{ 0 };
		unsigned int m_vboID{ 0 };
		unsigned int m_eboID{ 0 };
		void SetupMesh();

	};
}
#endif // !SRC_GRAPHICS_MESH_MESH_H
