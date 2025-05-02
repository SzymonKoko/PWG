#include "vertexArray.h"

pwg::VertexArray::VertexArray()
{
	glGenVertexArrays(1, &m_vertexArrayObjectID);
}

void pwg::VertexArray::LinkVertexBufferObject(VertexBuffer& VBO, GLuint layout)
{
	VBO.Bind();
	glVertexAttribPointer(layout, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
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
