#include "vertexBuffer.h"

pwg::VertexBuffer::VertexBuffer(std::vector<GLfloat>& vertices)
{
	glGenBuffers(1, &m_vertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	
	float offset = 1.0f;
	float heigth = 1;

	const int CHUNK_SIZE = 256;
	const int HALF_CHUNK_SIZE = CHUNK_SIZE / 2;
	for (float x = -HALF_CHUNK_SIZE; x < HALF_CHUNK_SIZE; x++) {
		for (float z = -HALF_CHUNK_SIZE; z < HALF_CHUNK_SIZE; z++) {
			float y = m_noise.GenerateHeigthMap(x, z) * 50.0f;
			m_instancePositions.emplace_back(glm::vec3(x, y, z));
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
