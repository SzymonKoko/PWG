#ifndef SRC_CORE_RENDERER_RENDERER_H_
#define SRC_CORE_RENDERER_RENDERER_H_

#include <glad.h>
#include "graphics/mesh/imesh.h"


namespace pwg
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		void Clear();
		void Draw(IMesh& mesh);
	private:


	};
} //namespace pwg
#endif // !SRC_CORE_RENDERER_RENDERER_H_
