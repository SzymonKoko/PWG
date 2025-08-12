#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/mesh/imesh.h"
#include "graphics/shader/shader.h"
#include "graphics/mesh/pyramidMesh.h"
#include "graphics/texture/texture.h"
#include "graphics/camera/icamera.h"

namespace pwg
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Clear();
		void Update();
		void Draw();

		void SetCamera(ICamera* camera);

		
		unsigned int GetShaderProgramID();
	private:
		Shader* m_shaderProgram;
		PyramidMesh m_pyramidMesh;
		Texture m_texDirt;
		ICamera* m_camera = nullptr;


		void InitShaders();
		void InitTextures();

	};
} //namespace pwg
#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
