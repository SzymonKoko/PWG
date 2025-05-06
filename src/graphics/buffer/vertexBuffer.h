#ifndef SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_
#define SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_

#include <glad.h>
#include <vector>

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

	private:
		GLuint m_vertexBufferObjectID{ 0 };
	};
} // namespace pwg
#endif // !SRC_GRAPHICS_BUFFER_VERTEX_BUFFER_H_
