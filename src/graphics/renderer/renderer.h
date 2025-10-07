#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "scene/components/cameraComponent.h"
#include "graphics/mesh/mesh.h"
#include "resources/resourceManager.h"

namespace pwg
{
	class Renderer
	{
	public:
		Renderer(std::shared_ptr<ResourceManager> resourceManager);
		~Renderer();

		void Clear();
		void Update(pwg::components::CameraComponent* camera, Mesh& mesh);
		void Draw(Mesh& mesh);

	private:
		std::shared_ptr<ResourceManager> m_resourceManager;
		std::shared_ptr<Shader> m_currentShader{ nullptr };
	};
} //namespace pwg
#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
