#include <glad/glad.h>
#include "frameBuffer.h"

pwg::FrameBuffer::FrameBuffer(int width, int height, bool useRBO, TextureType colorType)
	: m_width(width), m_height(height), m_useRBO(useRBO), m_colorType(colorType)
{
	m_fboTexture = std::make_shared<Texture>(m_width, m_height, m_colorType);
	InitializeFrameBuffer();
	PWG_INFO("Framebuffer created ({0}x{1})", width, height);
}

pwg::FrameBuffer::FrameBuffer(const pwg::FrameBuffer& otherFBO)
	: m_width(otherFBO.m_width),
	m_height(otherFBO.m_height),
	m_useRBO(otherFBO.m_useRBO),
	m_colorType(otherFBO.m_colorType)
{
	m_fboTexture = std::make_shared<Texture>(m_width, m_height, m_colorType);

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
	m_colorType = otherFBO.m_colorType;

	m_fboTexture = std::make_shared<Texture>(m_width, m_height, m_colorType);

	InitializeFrameBuffer();

	return *this;
}

pwg::FrameBuffer::~FrameBuffer()
{
	CleanUp();
}

void pwg::FrameBuffer::InitializeFrameBuffer()
{
	glViewport(0, 0, m_width, m_height);
	glGenFramebuffers(1, &m_frameBufferObjectID);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObjectID);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture->GetTextureID(), 0);

	if (m_useRBO)
	{
		glGenTextures(1, &m_depthTextureID);
		glBindTexture(GL_TEXTURE_2D, m_depthTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureID, 0);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		PWG_ERROR("Framebuffer is incomplete ({0}x{1})", m_width, m_height);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void pwg::FrameBuffer::CleanUp()
{
	glDeleteFramebuffers(1, &m_frameBufferObjectID);
	if (m_useRBO)
	{
		glDeleteTextures(1, &m_depthTextureID);
		m_depthTextureID = 0;
	}
	if (m_fboTexture)
	{
		GLuint textureID = m_fboTexture->GetTextureID();
		glDeleteTextures(1, &textureID);
	}
}

void pwg::FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferObjectID);
	glViewport(0, 0, m_width, m_height);
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
	if (width <= 0 || height <= 0) {
		return;
	}

	m_width = width;
	m_height = height;

	CleanUp(); 

	m_fboTexture = std::make_shared<Texture>(m_width, m_height, m_colorType);

	InitializeFrameBuffer();  
	PWG_DEBUG("Changed framebuffer size to {0}x{1}", m_width, m_height);
}

