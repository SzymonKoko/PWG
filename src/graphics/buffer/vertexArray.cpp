#include "vertexArray.h"

pwg::VertexArray::VertexArray()
{
	glGenVertexArrays(1, &m_vertexArrayObjectID);
}

void pwg::VertexArray::LinkVertexBufferObject(VertexBuffer& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
{
	VBO.Bind();
	glBindVertexArray(m_vertexArrayObjectID);
	if (layout < 3) {
		glEnableVertexAttribArray(layout);
		glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
	}
	else if (layout == 3) {
		GLuint instanceVBO = VBO.GetInstancedVBO();
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glEnableVertexAttribArray(layout);
		glVertexAttribPointer(layout, numComponents, type, GL_FALSE, sizeof(glm::vec3), offset);
		glVertexAttribDivisor(layout, 1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	glBindVertexArray(0);

	VBO.Unbind();

}

void pwg::VertexArray::Bind()
{
	glBindVertexArray(m_vertexArrayObjectID);
}

void pwg::VertexArray::Unbind()
{
	glBindVertexArray(0);
}

void pwg::VertexArray::Delete()
{
	glDeleteVertexArrays(1, &m_vertexArrayObjectID);
}
