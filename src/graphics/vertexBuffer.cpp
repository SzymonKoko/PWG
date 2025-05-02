#include "vertexBuffer.h"

pwg::VertexBuffer::VertexBuffer(GLfloat* vertices, GLsizeiptr size)
{
	glGenBuffers(1, &m_vertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

void pwg::VertexBuffer::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObjectID);
}

void pwg::VertexBuffer::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void pwg::VertexBuffer::Delete()
{
	glDeleteBuffers(1, &m_vertexBufferObjectID);
}
