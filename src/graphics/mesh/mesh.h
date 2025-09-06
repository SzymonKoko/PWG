#ifndef SRC_GRAPHICS_MESH_MESH_H
#define SRC_GRAPHICS_MESH_MESH_H

#include <vector>
#include "graphics/buffer/vertexArray.h"
#include "graphics/buffer/vertexBuffer.h"
#include "graphics/buffer/indexBuffer.h"

namespace pwg
{
	class Mesh 
	{
	public:
		Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		void Draw();
	private:


	};
}
#endif // !SRC_GRAPHICS_MESH_MESH_H
