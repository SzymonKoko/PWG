#include "triangleMesh.h"

pwg::TriangleMesh::TriangleMesh()
{
	m_indexCount = m_indices.size();
	m_vertexCount = m_vertices.size() / 3;

	VAO.Bind();

	//Creating Vertex buffer with vertices
	VertexBuffer VBO(m_vertices);

	//Creating index buffer with indices
	IndexBuffer EBO(m_indices);
	VAO.LinkVertexBufferObject(VBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

	Logger::LogInfo(Logger::Module::TriangleMesh, "Triangle mesh created");

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

unsigned int pwg::TriangleMesh::GetVertexCount()
{
	return m_vertexCount;
}
