#ifndef SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_
#define SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "core/noise/noise.h"

namespace pwg
{
	class VertexBuffer
	{
	public:

		// Constructor that generates a Vertex Buffer Object and links it to vertices
		VertexBuffer(std::vector<GLfloat>& vertices);
		~VertexBuffer() = default;

		// Binds the VBO
		void Bind();
		// Unbinds the VBO
		void Unbind();
		// Deletes the VBO
		void Delete();

		unsigned int GetInstancedPositionsSize();
		GLuint GetInstancedVBO();

	private:
		Noise m_noise;
		GLuint m_vertexBufferObjectID{ 0 };
		std::vector<glm::vec3> m_instancePositions;
		unsigned int instancedVBO;
	};
} // namespace pwg
#endif // !SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_
