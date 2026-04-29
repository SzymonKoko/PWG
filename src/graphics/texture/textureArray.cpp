#include <glad/glad.h>
#include "textureArray.h"

pwg::TextureArray::TextureArray(int width, int height, int numberOfLayers, TextureType type)
	: m_width(width),
	  m_height(height),
	  m_numberOfLayers(numberOfLayers),
	  m_textureType(type)
{
	
	m_wrapS = ToGL(TextureWrapMode::REPEAT);
	m_wrapT = ToGL(TextureWrapMode::REPEAT);

	m_minFilter = ToGL(TextureFilterMode::MIPMAP_LINEAR_LINEAR);
	m_magFilter = ToGL(TextureFilterMode::LINEAR);

	m_format = GetFormatForType(type);

	glGenTextures(1, &m_textureArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayID);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, m_format.internalFormat, m_width, m_height, m_numberOfLayers, 0, m_format.format, m_format.type, nullptr);

	//Set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_magFilter);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void pwg::TextureArray::SetLayerData(int layer, unsigned char* data)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayID);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, m_width, m_height, 1, m_format.format, m_format.type, data);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void pwg::TextureArray::SubmitTextureArray()
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayID);

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void pwg::TextureArray::LoadTexturesIntoArray(std::vector<std::string> texturePaths)
{
	stbi_set_flip_vertically_on_load(true);

	for (int i = 0; i < m_numberOfLayers; i++)
	{
		int width = 0;
		int height = 0;
		int nrChannels = 0;

		unsigned char* data = nullptr;

		if (m_textureType == TextureType::ALBEDO || m_textureType == TextureType::NORMAL)
		{
			data = stbi_load(texturePaths[i].c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		}
		else
		{
			data = stbi_load(texturePaths[i].c_str(), &width, &height, &nrChannels, STBI_grey);
		}

		if (!data)
		{
			PWG_ERROR("Failed to load texture: {}", texturePaths[i]);
			continue;
		}

		SetLayerData(i, data);

		stbi_image_free(data);
	}

	SubmitTextureArray();
}

void pwg::TextureArray::Bind(int slot)
{
	m_slot = slot;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayID);
}
