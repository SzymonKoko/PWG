#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/logger/logger.h"
#include "mesh.h"

namespace pwg
{
	Mesh::Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices, float size)
		:m_vertices(vertices),
		 m_indices(indices),
		 m_meshSize(size)
	{
		m_transform.position = { 0.0f, 0.0f, 0.0f };
		m_transform.rotation = { 0.0f, 0.5f, 0.0f };
		SetupMesh();
	}

	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &m_vaoID);
		glDeleteBuffers(1, &m_vboID);
		glDeleteBuffers(1, &m_eboID);
	}

	void Mesh::Update()
	{
		m_model = glm::mat4(1.0f);
		m_model = glm::translate(m_model, m_transform.position);

	}

	void Mesh::Draw()
	{
		glBindVertexArray(m_vaoID);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	std::vector<Vertex>& Mesh::GetVertices()
	{
		return m_vertices;
	}

	glm::mat4 Mesh::GetModelMatrix()
	{
		return m_model;
	}

	glm::vec3 Mesh::GetPosition()
	{
		return m_transform.position;
	}

	int Mesh::GetVertexCount()
	{
		return m_vertices.size();
	}

	void Mesh::SetPosition(glm::vec3 position)
	{
		m_transform.position = position;
		m_model = glm::mat4(1.0f);
		m_model = glm::translate(m_model, m_transform.position);
	}

	void Mesh::SetVertices(std::vector<Vertex>& vertices)
	{
		m_vertices = vertices;
		glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Mesh::UpdateMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int& size)
	{
		//RecalculateNormals();
		m_vertices = vertices;
		glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		m_indices = indices;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboID);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size() * sizeof(uint32_t), m_indices.data());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_meshSize = size;
	}

	int Mesh::GetSize()
	{
		return m_meshSize;
	}

	void Mesh::SetupMesh()
	{
		glGenVertexArrays(1, &m_vaoID);
		glGenBuffers(1, &m_vboID);
		glGenBuffers(1, &m_eboID);

		glBindVertexArray(m_vaoID);

		glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
		glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		PWG_INFO("Mesh uploaded to GPU");
	}
}

