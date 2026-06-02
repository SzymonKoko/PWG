#include <glad/glad.h>
#include "texture.h"


pwg::Texture::Texture(TextureType type)
{
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	m_textureType = type;
	m_format = GetFormatForType(type);
}

pwg::Texture::Texture(const std::string& imagePath, TextureType type)
{
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	m_textureType = type;
	m_format = GetFormatForType(type);

	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	m_image = stbi_load(imagePath.c_str(), &m_width, &m_height, &m_nrChannels, STBI_rgb_alpha);

	if (m_image)
	{

		GLenum format = (m_nrChannels == 4 ? GL_RGBA : GL_RGB);

		glTexImage2D(GL_TEXTURE_2D, 0, m_format.internalFormat, m_width, m_height, 0, m_format.format, m_format.type, m_image);

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

pwg::Texture::Texture(int width, int height, TextureType type)
{
	m_width = width;
	m_height = height;

	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	m_textureType = type;
	m_format = GetFormatForType(type);

	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);

	glTexImage2D(GL_TEXTURE_2D, 0, m_format.internalFormat, width, height, 0, m_format.format, m_format.type, NULL);

	PWG_DEBUG("Texture created ({0}x{1})", width, height);

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
	//PWG_DEBUG("TEXTURE ID: {0}", m_textureID);

	m_slot = slot;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	//PWG_DEBUG("Texture binded (id={0})", m_textureID);
}

void pwg::Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void pwg::Texture::ClearTexture()
{
	glClearTexImage(m_textureID, 0, m_format.format, m_format.type, nullptr);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void pwg::Texture::ClearTexture(float value)
{
	float color[4] = { value, value, value, value };
	glClearTexImage(m_textureID, 0, m_format.format, m_format.type, color);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void pwg::Texture::CopyFrom(const Texture& source)
{
	if (m_width != source.m_width || m_height != source.m_height)
	{
		PWG_ERROR("Cannot copy texture data: texture sizes do not match");
		return;
	}
	if (m_textureType != source.m_textureType)
	{
		PWG_ERROR("Cannot copy texture data: texture formats do not match");
		return;
	}

	glCopyImageSubData(
		source.m_textureID, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_textureID, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_width, m_height, 1
	);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

std::vector<float> pwg::Texture::ReadFloatData()
{
	int components = 1;
	if (m_format.format == GL_RG)
	{
		components = 2;
	}
	else if (m_format.format == GL_RGB)
	{
		components = 3;
	}
	else if (m_format.format == GL_RGBA)
	{
		components = 4;
	}

	std::vector<float> pixels(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * components, 0.0f);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glGetTexImage(GL_TEXTURE_2D, 0, m_format.format, GL_FLOAT, pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	return pixels;
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

	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		m_nrChannels = nrChannels;
		glTexImage2D(GL_TEXTURE_2D, 0, m_format.internalFormat, m_width, m_height, 0, m_format.format, m_format.type, data);
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format.format, m_format.type, data);
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

void pwg::Texture::UpdateFloatData(const std::vector<float>& data, int width, int height)
{
	if (data.empty())
	{
		return;
	}

	glBindTexture(GL_TEXTURE_2D, m_textureID);

	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		glTexImage2D(GL_TEXTURE_2D, 0, m_format.internalFormat, m_width, m_height, 0, m_format.format, GL_FLOAT, data.data());
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format.format, GL_FLOAT, data.data());
	}

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindTexture(GL_TEXTURE_2D, 0);
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

