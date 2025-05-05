#include "mesh.h"

pwg::Mesh::Mesh(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
	:m_vertices(vertices), m_indices(indices)
{

	VAO.Bind();

	//Creating Vertex buffer with vertices
	VertexBuffer VBO(m_vertices);

	//Creating index buffer with indices
	IndexBuffer EBO(m_indices);
	VAO.LinkVertexBufferObject(VBO, 0);

	std::cout << "Mesh created: " << m_vertices.size() << ", " << m_indices.size();

	//Unbinding buffers
	VAO.Unbind();
	EBO.Unbind();
	VBO.Unbind();

	//Deleting index and vertex buffers
	EBO.Delete();
	VBO.Delete();
}

pwg::Mesh::~Mesh()
{
	VAO.Delete();
}

