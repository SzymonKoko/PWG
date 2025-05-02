#include "indexBuffer.h"

pwg::IndexBuffer::IndexBuffer(GLfloat* indices, GLsizeiptr size)
{
	glGenBuffers(1, &m_indexBufferObjectID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObjectID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

void pwg::IndexBuffer::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObjectID);
}

void pwg::IndexBuffer::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void pwg::IndexBuffer::Delete()
{
	glDeleteBuffers(1, &m_indexBufferObjectID);
}
