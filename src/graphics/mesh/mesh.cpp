#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "mesh.h"

namespace pwg
{
	Mesh::Mesh(const std::vector<pwg::Vertex>& vertices, const std::vector<unsigned int>& indices)
		:m_vertices(vertices),
		 m_indices(indices)
	{
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
		glm::mat4 projection = glm::mat4(1.0f);

		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

		glUseProgram(shaderID);

		int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	}

	void Mesh::Draw()
	{
		glBindVertexArray(m_vaoID);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
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

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}
}

