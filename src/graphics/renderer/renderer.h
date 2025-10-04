#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "scene/components/cameraComponent.h"
#include "graphics/mesh/mesh.h"

namespace pwg
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Clear();
		void Update(pwg::components::CameraComponent* camera, Mesh& mesh);
		void Draw(Mesh& mesh);

		
		unsigned int GetShaderProgramID();
	private:
		Shader* m_shaderProgram;
		Texture m_texDirt;

		void InitShaders();
		void InitTextures();

	};
} //namespace pwg
#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
