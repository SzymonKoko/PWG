#ifndef MESH_H_
#define MESH_H_

#include <glad.h>
#include <vector>
#include "shader.h"
#include "vertexArray.h"
#include "indexBuffer.h"

namespace pwg
{
	class Mesh
	{
	public:
		Mesh(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);
		~Mesh() = default;

		VertexArray VAO;

		void Draw(Shader& shader);
	private:
		std::vector<GLfloat>& m_vertices;
		std::vector<GLuint>& m_indices;

	};
} // namespace pwg
#endif // !MESH_H_
