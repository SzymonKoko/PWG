#ifndef SRC_GRAPHICS_MESH_PYRAMID_MESH_H_
#define SRC_GRAPHICS_MESH_PYRAMID_MESH_H_

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
			-0.5f, -0.5f,  0.5f, //0
			-0.5f, -0.5f, -0.5f, //1
			 0.5f, -0.5f, -0.5f, //2
			 0.5f, -0.5f,  0.5f, //3
			-0.5f,  0.5f,  0.5f, //4
			-0.5f,  0.5f, -0.5f, //5
			 0.5f,  0.5f, -0.5f, //6
			 0.5f,  0.5f,  0.5f, //7

		};

		std::vector<GLuint> m_indices =
		{
			0, 1, 2, //Down
			0, 2, 3,

			0, 1, 5, //Left
			0, 4, 5,

			0, 3, 7, //Front
			0, 4, 7,

			1, 2, 6, //Back
			1, 5, 6,

			3, 2, 6, //Right
			3, 7, 6,

			4, 5, 6, //Up
			4, 6, 7,

		};
		unsigned int m_indexCount{ 0 };
		unsigned int m_vertexCount{ 0 };
	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_PYRAMID_MESH_H_
