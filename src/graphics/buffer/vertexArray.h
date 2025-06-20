#ifndef SRC_GRAPHICS_BUFFER_VERTEX_ARRAY_H_
#define SRC_GRAPHICS_BUFFER_VERTEX_ARRAY_H_

#include <glad.h>
#include <iostream>
#include "vertexBuffer.h"

namespace pwg
{
	class VertexArray
	{
	public:

		// Constructor that generates a VAO ID
		VertexArray();
		~VertexArray() = default;

		// Links a VBO to the VAO using a certain layout
		void LinkVertexBufferObject(VertexBuffer& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);
		// Binds the VAO
		void Bind();
		// Unbinds the VAO
		void Unbind();
		// Deletes the VAO
		void Delete();
	private:
		GLuint m_vertexArrayObjectID{ 0 };
	};
	
} // namespace pwg
#endif // !SRC_GRAPHICS_BUFFER_VERTEX_ARRAY_H_
