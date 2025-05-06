#include "pyramidMesh.h"

pwg::PyramidMesh::PyramidMesh()
{
	m_indexCount = m_indices.size();
	m_vertexCount = m_vertices.size() / 3;

	VAO.Bind();

	//Creating Vertex buffer with vertices
	VertexBuffer VBO(m_vertices);

	//Creating index buffer with indices
	IndexBuffer EBO(m_indices);
	VAO.LinkVertexBufferObject(VBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

	Logger::LogInfo(Logger::Module::PyramidMesh, "Pyramid mesh created");

	//Unbinding buffers
	VAO.Unbind();
	EBO.Unbind();
	VBO.Unbind();

	//Deleting index and vertex buffers
	EBO.Delete();
	VBO.Delete();
}

pwg::PyramidMesh::~PyramidMesh()
{
	VAO.Delete();
}

void pwg::PyramidMesh::Bind()
{
	VAO.Bind();
}

void pwg::PyramidMesh::Unbind()
{
	VAO.Unbind();
}

unsigned int pwg::PyramidMesh::GetIndexCount()
{
	return m_indexCount;
}

unsigned int pwg::PyramidMesh::GetVertexCount()
{
	return m_vertexCount;
}

void pwg::PyramidMesh::InitMesh(const unsigned int& shaderID)
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	model = glm::rotate(model, (float)glfwGetTime() * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, -0.5f, -4.0f));
	projection = glm::perspective(glm::radians(45.0f), (float)(800 / 800), 0.1f, 100.0f);

	glUseProgram(shaderID);

	int modelLocation = glGetUniformLocation(shaderID, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

	int viewLocation = glGetUniformLocation(shaderID, "view");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

	int projectionLocation = glGetUniformLocation(shaderID, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

	
}
