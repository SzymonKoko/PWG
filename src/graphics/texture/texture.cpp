#include <glad/glad.h>
#include "texture.h"


pwg::Texture::Texture()
{
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);
}

pwg::Texture::Texture(const std::string& imagePath)
{
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	m_image = stbi_load(imagePath.c_str(), &m_width, &m_height, &m_nrChannels, 0);

	if (m_image)
	{

		GLenum format = (m_nrChannels == 4 ? GL_RGBA : GL_RGB);

		glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, m_image);

		if (m_hasMipmap)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
	else
	{
		PWG_ERROR("Failed to load texture ({0})", imagePath.c_str());
	}

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);

	if (m_image)
	{
		stbi_image_free(m_image);
		m_image = nullptr;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

pwg::Texture::Texture(int width, int height)
{
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	PWG_DEBUG("Framebuffer texture created ({0}x{1})", width, height);

	glBindTexture(GL_TEXTURE_2D, 0);
}

pwg::Texture::~Texture()
{

	if (m_textureID != 0)
	{
		//glDeleteTextures(1, &m_textureID);
	}
}

void pwg::Texture::Bind(int slot)
{
	m_slot = slot;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	PWG_DEBUG("Texture binded (id={0})", m_textureID);
}

void pwg::Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}


void pwg::Texture::SetTextureWrapping(int wrapS, int wrapT)
{
	m_wrapS = wrapS;
	m_wrapT = wrapT;

	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void pwg::Texture::SetTextureFiltering(int minFilter, int magFilter)
{
	m_minFilter = minFilter;
	m_magFilter = magFilter;

	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void pwg::Texture::UpdateData(unsigned char* data, int width, int height, int nrChannels)
{
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	GLenum format = (m_nrChannels == 4 ? GL_RGBA : GL_RGB);

	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		m_nrChannels = nrChannels;
		glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, format, GL_UNSIGNED_BYTE, data);
	}

	if (m_hasMipmap)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);
}

unsigned int pwg::Texture::GetTextureID()
{
	return m_textureID;
}

int pwg::Texture::GetWidth()
{
	return m_width;
}

int pwg::Texture::GetHeight()
{
	return m_height;
}

