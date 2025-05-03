#include "mesh.h"

pwg::Mesh::Mesh(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
	:m_vertices(vertices), m_indices(indices)
{

	VAO.Bind();
	VertexBuffer VBO(m_vertices);

	IndexBuffer EBO(m_indices);
	VAO.LinkVertexBufferObject(VBO, 0);

	std::cout << "Mesh created: " << m_vertices.size() << ", " << m_indices.size();

	VAO.Unbind();
	EBO.Unbind();
	VBO.Unbind();
}

void pwg::Mesh::Draw(Shader& shader)
{
	shader.ActivateShader();
	VAO.Bind();

	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
}
