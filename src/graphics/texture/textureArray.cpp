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

pwg::TextureArray::~TextureArray()
{
	if (m_textureArrayID != 0)
	{
		glDeleteTextures(1, &m_textureArrayID);
	}
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

bool pwg::TextureArray::LoadTexturesIntoArray(
	const std::vector<std::string>& texturePaths,
	const std::vector<unsigned char>& fallbackValues,
	std::string* error)
{
	if (static_cast<int>(texturePaths.size()) != m_numberOfLayers)
	{
		if (error) *error = "Texture layer count mismatch";
		return false;
	}
	if (!fallbackValues.empty() && fallbackValues.size() != texturePaths.size())
	{
		if (error) *error = "Texture fallback count mismatch";
		return false;
	}

	stbi_set_flip_vertically_on_load(true);

	for (int i = 0; i < m_numberOfLayers; i++)
	{
		if (texturePaths[i].empty())
		{
			const unsigned char fallback = fallbackValues.empty() ? 255u : fallbackValues[i];
			const int channels = (m_textureType == TextureType::ALBEDO || m_textureType == TextureType::NORMAL) ? 4 : 1;
			std::vector<unsigned char> pixels(static_cast<size_t>(m_width) * m_height * channels, fallback);
			if (m_textureType == TextureType::NORMAL && channels == 4)
			{
				for (size_t pixel = 0; pixel < pixels.size(); pixel += 4)
				{
					pixels[pixel] = 128;
					pixels[pixel + 1] = 128;
					pixels[pixel + 2] = 255;
					pixels[pixel + 3] = 255;
				}
			}
			SetLayerData(i, pixels.data());
			continue;
		}

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
			if (error) *error = "Failed to load texture: " + texturePaths[i];
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return false;
		}
		if (width != m_width || height != m_height)
		{
			stbi_image_free(data);
			if (error) *error = "Texture size mismatch: " + texturePaths[i];
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return false;
		}

		SetLayerData(i, data);

		stbi_image_free(data);
	}

	SubmitTextureArray();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return true;
}

void pwg::TextureArray::Bind(int slot)
{
	m_slot = slot;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayID);
}
