#ifndef SRC_GRAPHICS_RENDERER_IRENDERABLE_H_
#define SRC_GRAPHICS_RENDERER_IRENDERABLE_H_

#include "graphics/renderer/renderer.h"

namespace pwg
{

	class IRenderable
	{
	public:
		virtual void Update(float dt) = 0;
		virtual void Draw(Renderer& renderer) = 0;

		virtual std::shared_ptr<Mesh> GetMesh() = 0;
		virtual std::shared_ptr<Material> GetMaterial() = 0;

	protected:
	};
}
#endif // !SRC_GRAPHICS_RENDERER_IRENDERABLE_H_
