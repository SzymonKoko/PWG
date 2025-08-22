#include <glad/glad.h>
#include "texture.h"


pwg::Texture::Texture()
{
	
}

pwg::Texture::~Texture()
{

	if (m_textureID != 0)
	{
		glDeleteTextures(1, &m_textureID);
	}
}

void pwg::Texture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void pwg::Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void pwg::Texture::LoadFromFile(const std::string& imagePath)
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	m_image = stbi_load(imagePath.c_str(), &m_width, &m_height, &m_nrChannels, 0);

	if (m_image)
	{
		if (m_nrChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_image);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else if (m_nrChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_image);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
	}
	else
	{
		Logger::LogError(Logger::Module::Texture, "Failed to load texture");
	}

	if (m_image)
	{
		stbi_image_free(m_image);
		m_image = nullptr;
	}
}

void pwg::Texture::LoadFramebufferTexture(int width, int height)
{
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

}
