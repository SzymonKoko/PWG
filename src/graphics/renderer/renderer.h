#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/mesh/imesh.h"
#include "graphics/shader/shader.h"
#include "graphics/mesh/pyramidMesh.h"
#include "graphics/texture/texture.h"
#include "scene/components/cameraComponent.h"

namespace pwg
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Clear();
		void Update(pwg::components::CameraComponent* camera);
		void Draw();

		
		unsigned int GetShaderProgramID();
	private:
		Shader* m_shaderProgram;
		PyramidMesh m_pyramidMesh;
		Texture m_texDirt;

		void InitShaders();
		void InitTextures();

	};
} //namespace pwg
#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
