#include "vertexArray.h"

pwg::VertexArray::VertexArray()
{
	glGenVertexArrays(1, &m_vertexArrayObjectID);
}

void pwg::VertexArray::LinkVertexBufferObject(VertexBuffer& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
{
	VBO.Bind();
	glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
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
