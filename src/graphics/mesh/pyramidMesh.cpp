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
	VAO.LinkVertexBufferObject(VBO, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO.LinkVertexBufferObject(VBO, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO.LinkVertexBufferObject(VBO, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	VAO.LinkVertexBufferObject(VBO, 3, 3, GL_FLOAT, 3 * sizeof(glm::vec3), (void*)0);

	m_instancedPositionsCount = VBO.GetInstancedPositionsSize();

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

unsigned int pwg::PyramidMesh::GetInstancedPositionsSize()
{
	return m_instancedPositionsCount;
}

void pwg::PyramidMesh::Update(const unsigned int& shaderID)
{
	glm::vec3 start_pos(0.0f, 0.0f, 0.0f);
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	//model = glm::rotate(model, (float)glfwGetTime()* 1 * glm::radians(15.0f), glm::vec3(0.0f, 0.1f, 0.0f));

	projection = glm::perspective(glm::radians(45.0f), (float)(800 / 800), 0.1f, 300.0f);

	glUseProgram(shaderID);

	int modelLocation = glGetUniformLocation(shaderID, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

	int projectionLocation = glGetUniformLocation(shaderID, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	
}
