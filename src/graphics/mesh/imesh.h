#ifndef SRC_GRAPHICS_MESH_IMESH_H_
#define SRC_GRAPHICS_MESH_IMESH_H_



#include "graphics/buffer/vertexArray.h"
#include "graphics/buffer/indexBuffer.h"
#include "core/logger/logger.h"

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

namespace pwg
{
	class IMesh
	{
	public:
		virtual ~IMesh() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual unsigned int GetIndexCount() = 0;
		virtual unsigned int GetVertexCount() = 0;
	private:

	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_IMESH_H_
