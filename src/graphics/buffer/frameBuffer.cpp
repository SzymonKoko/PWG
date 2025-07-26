#include <glad/glad.h>
#include "frameBuffer.h"

pwg::FrameBuffer::FrameBuffer(int width, int height, bool useRBO)
	: m_width(width), m_height(height), m_useRBO(useRBO)
{
	m_fboTexture = std::make_shared<Texture>();
	m_fboTexture->LoadFramebufferTexture(m_width, m_height);
	InitializeFrameBuffer();

}

pwg::FrameBuffer::FrameBuffer(const pwg::FrameBuffer& otherFBO)
	: m_width(otherFBO.m_width),
	  m_height(otherFBO.m_height),
	  m_useRBO(otherFBO.m_useRBO),
	  m_resizeable(otherFBO.m_resizeable)
{
	m_fboTexture = std::make_shared<Texture>();
	m_fboTexture->LoadFramebufferTexture(m_width, m_height);

	InitializeFrameBuffer();
}

pwg::FrameBuffer& pwg::FrameBuffer::operator=(const FrameBuffer& otherFBO)
{
	if (this == &otherFBO)
	{
		return *this;
	}

	CleanUp();

	m_width = otherFBO.m_width;
	m_height = otherFBO.m_height;
	m_useRBO = otherFBO.m_useRBO;
	m_resizeable = otherFBO.m_resizeable;

	m_fboTexture = std::make_shared<Texture>();
	m_fboTexture->LoadFramebufferTexture(m_width, m_height);

	InitializeFrameBuffer();

	return *this;
}

pwg::FrameBuffer::~FrameBuffer()
{
	CleanUp();
}

void pwg::FrameBuffer::InitializeFrameBuffer()
{
	glGenFramebuffers(1, &m_frameBufferObjectID);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObjectID);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture->GetTextureID(), 0);

	if (m_useRBO)
	{
		glGenRenderbuffers(1, &m_renderBufferObjectID);
		glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferObjectID);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_width, m_height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBufferObjectID);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void pwg::FrameBuffer::CleanUp()
{
	glDeleteFramebuffers(1, &m_frameBufferObjectID);
	if (m_useRBO)
	{
		glDeleteRenderbuffers(1, &m_renderBufferObjectID);
	}
	GLuint textureID = m_fboTexture->GetTextureID();
	std::cout << m_fboTexture->GetTextureID() << std::endl;
	glDeleteTextures(1, &textureID);
}

void pwg::FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObjectID);
}

void pwg::FrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void pwg::FrameBuffer::Delete()
{
	CleanUp();
}

void pwg::FrameBuffer::Resize(int width, int height)
{
	m_width = width;
	m_height = height;
	m_resizeable = true;
}

void pwg::FrameBuffer::CheckResize()
{
	if (!m_resizeable)
	{
		return;
	}

	CleanUp();

	m_fboTexture = std::make_shared<Texture>();
	m_fboTexture->LoadFramebufferTexture(m_width, m_height);

	InitializeFrameBuffer();
	m_resizeable = false;

}
