#ifndef VERTEX_BUFFER_H_
#define VERTEX_BUFFER_H_

#include <glad.h>

namespace pwg
{
	class VertexBuffer
	{
	public:

		// Constructor that generates a Vertex Buffer Object and links it to vertices
		VertexBuffer(GLfloat* vertices, GLsizeiptr size);
		~VertexBuffer() = default;

		// Binds the VBO
		void Bind();
		// Unbinds the VBO
		void Unbind();
		// Deletes the VBO
		void Delete();

	private:
		GLuint m_vertexBufferObjectID{ 0 };
	};
} // namespace pwg
#endif // !VERTEX_BUFFER_H_
