#ifndef SRC_GRAPHICS_MESH_TRIANGLE_MESH_H_
#define SRC_GRAPHICS_MESH_TRIANGLE_MESH_H_

#include "imesh.h"

namespace pwg
{
	class TriangleMesh : public IMesh
	{
	public:
		TriangleMesh();
		~TriangleMesh();

		void Bind() override;
		void Unbind() override;
		unsigned int GetIndexCount() override;
		unsigned int GetVertexCount() override;


	private:
		VertexArray VAO;

		std::vector<GLfloat> m_vertices =
		{
			-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower left corner
			0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower right corner
			0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f, // Upper corner
			-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner left
			0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner right
			0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f // Inner down
		};

		std::vector<GLuint> m_indices =
		{
			0, 3, 5, // Lower left triangle
			3, 2, 4, // Lower right triangle
			5, 4, 1 // Upper triangle
		};
		unsigned int m_indexCount{ 0 };
		unsigned int m_vertexCount{ 0 };
	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_TRIANGLE_MESH_H_
