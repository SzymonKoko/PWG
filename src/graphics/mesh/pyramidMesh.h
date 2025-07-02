#ifndef SRC_GRAPHICS_MESH_PYRAMID_MESH_H_
#define SRC_GRAPHICS_MESH_PYRAMID_MESH_H_


#include <glad.h>
#include "imesh.h"


namespace pwg
{
	class PyramidMesh : public IMesh
	{
	public:
		PyramidMesh();
		~PyramidMesh();

		void Bind() override;
		void Unbind() override;
		unsigned int GetIndexCount() override;
		unsigned int GetVertexCount() override;
		unsigned int GetInstancedPositionsSize();

		void Update(const unsigned int& shaderID);


	private:
		VertexArray VAO;
		unsigned int m_instancedPositionsCount{0};

		std::vector<GLfloat> m_vertices =
		{
			// Front face
		   -0.5f, -0.5f,  0.5f,   1, 0, 0,   0.0f, 0.0f, // Bottom-left
			0.5f, -0.5f,  0.5f,   1, 0, 0,   1.0f, 0.0f, // Bottom-right
			0.5f,  0.5f,  0.5f,   1, 0, 0,   1.0f, 1.0f, // Top-right
		   -0.5f,  0.5f,  0.5f,   1, 0, 0,   0.0f, 1.0f, // Top-left

			// Back face
			0.5f, -0.5f, -0.5f,   0, 1, 0,   0.0f, 0.0f,
		   -0.5f, -0.5f, -0.5f,   0, 1, 0,   1.0f, 0.0f,
		   -0.5f,  0.5f, -0.5f,   0, 1, 0,   1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,   0, 1, 0,   0.0f, 1.0f,

			// Left face
		   -0.5f, -0.5f, -0.5f,   0, 0, 1,   0.0f, 0.0f,
		   -0.5f, -0.5f,  0.5f,   0, 0, 1,   1.0f, 0.0f,
		   -0.5f,  0.5f,  0.5f,   0, 0, 1,   1.0f, 1.0f,
		   -0.5f,  0.5f, -0.5f,   0, 0, 1,   0.0f, 1.0f,

			// Right face
			0.5f, -0.5f,  0.5f,   1, 1, 0,   0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,   1, 1, 0,   1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,   1, 1, 0,   1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,   1, 1, 0,   0.0f, 1.0f,

			// Top face
		   -0.5f,  0.5f,  0.5f,   0, 1, 1,   0.0f, 0.0f,
			0.5f,  0.5f,  0.5f,   0, 1, 1,   1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,   0, 1, 1,   1.0f, 1.0f,
		   -0.5f,  0.5f, -0.5f,   0, 1, 1,   0.0f, 1.0f,

			// Bottom face
		   -0.5f, -0.5f, -0.5f,   1, 0, 1,   0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,   1, 0, 1,   1.0f, 0.0f,
			0.5f, -0.5f,  0.5f,   1, 0, 1,   1.0f, 1.0f,
		   -0.5f, -0.5f,  0.5f,   1, 0, 1,   0.0f, 1.0f,

		};

		std::vector<GLuint> m_indices =
		{
			// Front face
			0, 1, 2,
			0, 2, 3,

			// Back face
			4, 5, 6,
			4, 6, 7,

			// Left face
			8, 9, 10,
			8, 10, 11,

			// Right face
			12, 13, 14,
			12, 14, 15,

			// Top face
			16, 17, 18,
			16, 18, 19,

			// Bottom face
			20, 21, 22,
			20, 22, 23
		};
		unsigned int m_indexCount{ 0 };
		unsigned int m_vertexCount{ 0 };
	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_PYRAMID_MESH_H_
