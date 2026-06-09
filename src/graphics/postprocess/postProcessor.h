#ifndef SRC_GRAPHICS_POSTPROCESS_POST_PROCESSOR_H_
#define SRC_GRAPHICS_POSTPROCESS_POST_PROCESSOR_H_

#include "graphics/shader/shader.h"
#include "scene/environment/environmentSettings.h"

#include <memory>

namespace pwg
{
	class PostProcessor
	{
	public:
		explicit PostProcessor(std::shared_ptr<Shader> shader);
		~PostProcessor();

		void Render(unsigned int sourceTexture, const EnvironmentSettings& settings, int width, int height);

	private:
		void InitializeQuad();

		std::shared_ptr<Shader> m_shader;
		unsigned int m_vao{ 0 };
		unsigned int m_vbo{ 0 };
	};
}

#endif // SRC_GRAPHICS_POSTPROCESS_POST_PROCESSOR_H_
