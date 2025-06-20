#include "vertexBuffer.h"

pwg::VertexBuffer::VertexBuffer(std::vector<GLfloat>& vertices)
{
	glGenBuffers(1, &m_vertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	
	float offset = 1.0f;
	float heigth = 1;

	const int CHUNK_SIZE = 16;
	for (float x = 0; x < CHUNK_SIZE; x++) {
		for (float y = 0; y < heigth; y++) {
			for (float z = 0; z < CHUNK_SIZE; z++) {
				m_instancePositions.emplace_back(glm::vec3(x, y, z));
			}
		}
	}

	glGenBuffers(1, &instancedVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_instancePositions.size(), m_instancePositions.data(), GL_STATIC_DRAW);
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

unsigned int pwg::VertexBuffer::GetInstancedPositionsSize()
{
	return m_instancePositions.size();
}

GLuint pwg::VertexBuffer::GetInstancedVBO()
{
	return instancedVBO;
}
