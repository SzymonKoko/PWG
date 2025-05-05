#ifndef MESH_H_
#define MESH_H_

#include <glad.h>
#include <vector>
#include "graphics/shader.h"
#include "graphics/vertexArray.h"
#include "graphics/indexBuffer.h"

namespace pwg
{
	class Mesh
	{
	public:
		Mesh(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);
		~Mesh();

		VertexArray VAO;

	private:
		std::vector<GLfloat>& m_vertices;
		std::vector<GLuint>& m_indices;

	};
} // namespace pwg
#endif // !MESH_H_
