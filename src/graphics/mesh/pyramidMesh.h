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

		void InitMesh(const unsigned int& shaderID);


	private:
		VertexArray VAO;

		std::vector<GLfloat> m_vertices =
		{
			-0.5f, 0.0f,  0.5f, 
			-0.5f, 0.0f, -0.5f, 
			 0.5f, 0.0f, -0.5f, 
			 0.5f, 0.0f,  0.5f, 
			 0.0f, 0.8f,  0.0f
		};

		std::vector<GLuint> m_indices =
		{
			0, 1, 2,
			0, 2, 3,
			0, 1, 4,
			1, 2, 4,
			2, 3, 4,
			3, 0, 4
		};
		unsigned int m_indexCount{ 0 };
		unsigned int m_vertexCount{ 0 };
	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_PYRAMID_MESH_H_
