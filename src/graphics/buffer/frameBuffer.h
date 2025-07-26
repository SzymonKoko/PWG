#ifndef SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_
#define SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_

#include "graphics/texture/texture.h"
#include <vector>
#include <memory>

namespace pwg
{
	class FrameBuffer
	{
	public:

		FrameBuffer(int width, int height, bool useRBO);
		FrameBuffer(const pwg::FrameBuffer& otherFBO);
		FrameBuffer& operator=(const FrameBuffer& otherFBO);
		~FrameBuffer();

		void Bind();
		void Unbind();
		void Delete();

		void Resize(int width, int height);
		void CheckResize();

		unsigned int GetID() { return m_frameBufferObjectID; }
		unsigned int GetTextureID() { return m_fboTexture->GetTextureID(); }
		int GetWidth() { return m_width; }
		int GetHeight() { return m_height; }

	private:
		unsigned int m_frameBufferObjectID{ 0 };
		unsigned int m_renderBufferObjectID{ 0 };
		std::shared_ptr<Texture> m_fboTexture{ nullptr };
		int m_width{ 800 };
		int m_height{ 800 };
		bool m_resizeable{ false };
		bool m_useRBO{ false };

	private:
		void InitializeFrameBuffer();
		void CleanUp();

	};
} // namespace pwg
#endif // !SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_
