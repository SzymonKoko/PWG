#include "triangleMesh.h"

pwg::TriangleMesh::TriangleMesh()
{
	m_indexCount = m_indices.size();
	std::cout << "IndexCOunt: " << m_indexCount;

	VAO.Bind();

	//Creating Vertex buffer with vertices
	VertexBuffer VBO(m_vertices);

	//Creating index buffer with indices
	IndexBuffer EBO(m_indices);
	VAO.LinkVertexBufferObject(VBO, 0);

	std::cout << "Triangle Mesh created: " << m_vertices.size() << ", " << m_indices.size();

	//Unbinding buffers
	VAO.Unbind();
	EBO.Unbind();
	VBO.Unbind();

	//Deleting index and vertex buffers
	EBO.Delete();
	VBO.Delete();
}

pwg::TriangleMesh::~TriangleMesh()
{
	VAO.Delete();
}

void pwg::TriangleMesh::Bind()
{
	VAO.Bind();
}

void pwg::TriangleMesh::Unbind()
{
	VAO.Unbind();
}

unsigned int pwg::TriangleMesh::GetIndexCount()
{
	return m_indexCount;
}
