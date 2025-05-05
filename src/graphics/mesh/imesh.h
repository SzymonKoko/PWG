#ifndef SRC_GRAPHICS_MESH_IMESH_H_
#define SRC_GRAPHICS_MESH_IMESH_H_

#include <iostream>
#include <glad.h>
#include <vector>

#include "graphics/vertexArray.h"
#include "graphics/indexBuffer.h"

namespace pwg
{
	class IMesh
	{
	public:
		virtual ~IMesh() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual unsigned int GetIndexCount() = 0;
	private:

	};
} //namespace pwg
#endif // !SRC_GRAPHICS_MESH_IMESH_H_
