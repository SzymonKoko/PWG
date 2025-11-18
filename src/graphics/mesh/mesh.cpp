#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/logger/logger.h"
#include "mesh.h"

namespace pwg
{
	Mesh::Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices, int size)
		:m_vertices(vertices),
		 m_indices(indices),
		 m_meshSize(size)
	{
		//RecalculateNormals();
		SetupMesh();
	}

	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &m_vaoID);
		glDeleteBuffers(1, &m_vboID);
		glDeleteBuffers(1, &m_eboID);
	}

	void Mesh::Update(unsigned int shaderID)
	{
		glm::vec3 start_pos(0.0f, 0.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		
		if (m_modelUniformLocation == -1)
		{
			m_modelUniformLocation = glGetUniformLocation(shaderID, "model");
		}
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, (float)glfwGetTime() * glm::radians(10.0f), glm::vec3(0.0f, 0.5f, 0.0f));

		glUseProgram(shaderID);

		glUniformMatrix4fv(m_modelUniformLocation, 1, GL_FALSE, glm::value_ptr(model));
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

	void Mesh::RecalculateNormals()
	{
		for (auto& v : m_vertices)
			v.normal = glm::vec3(0.0f);

		glm::vec3 normal(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < m_indices.size(); i+= 3)
		{
			glm::vec3 A = m_vertices[m_indices[i]].position;
			glm::vec3 B = m_vertices[m_indices[i+1]].position;
			glm::vec3 C = m_vertices[m_indices[i+2]].position;

			glm::vec3 a = C - B;
			glm::vec3 b = A - B;

			normal = glm::normalize(glm::cross(a, b));

			m_vertices[m_indices[i]].normal += normal;
			m_vertices[m_indices[i+1]].normal += normal;
			m_vertices[m_indices[i+2]].normal += normal;
		}

		for (auto& v : m_vertices)
			v.normal = glm::normalize(v.normal);

		SetVertices(m_vertices);
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

