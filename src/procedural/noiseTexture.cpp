#include <glad/glad.h>
#include "noiseTexture.h"

pwg::NoiseTexture::NoiseTexture()
{
	m_noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);

	if (m_textureID != 0)
		glDeleteTextures(1, &m_textureID);

	glGenTextures(1, &m_textureID);

	UpdateNoiseData(m_noiseParams);
}

pwg::NoiseTexture::NoiseTexture(const NoiseParameters& noiseParams)
	: m_noiseParams(noiseParams)
{
	m_noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);

	if (m_textureID != 0)
		glDeleteTextures(1, &m_textureID);

	glGenTextures(1, &m_textureID);

	UpdateNoiseData(noiseParams);
}

pwg::NoiseTexture::NoiseTexture(const NoiseTexture& other)
	: m_noiseParams(other.m_noiseParams),
	  m_noise(other.m_noise),
	  m_pixels(other.m_pixels),
	  m_noiseData(other.m_noiseData)
{
	glGenTextures(1, &m_textureID);
	UploadToGPU(); 
}

pwg::NoiseTexture::~NoiseTexture()
{
	if (m_textureID)
	{
		glDeleteTextures(1, &m_textureID);
	}
}

void pwg::NoiseTexture::UpdateNoiseData(const NoiseParameters& noiseParams)
{
	m_noiseParams = noiseParams;
	m_noise.SetSeed(m_noiseParams.seed);
	m_noise.SetFrequency(m_noiseParams.frequency);
	GenerateNoiseData();
	UploadToGPU();
}

void pwg::NoiseTexture::SetAmplitude(float amplitude)
{
	m_noiseParams.amplitude = amplitude;
}

void pwg::NoiseTexture::SetFrequency(float frequency)
{
	m_noiseParams.frequency = frequency;
}

void pwg::NoiseTexture::SetScale(float scale)
{
	m_noiseParams.scale = scale;
}

void pwg::NoiseTexture::SetOffset(glm::vec2 offset)
{
	m_noiseParams.offset = offset;
}

void pwg::NoiseTexture::SetSeed(int seed)
{
	m_noiseParams.seed = seed;
}

void pwg::NoiseTexture::GenerateNoiseData()
{
	m_pixels.resize(m_noiseParams.width * m_noiseParams.height * 4);
	m_noiseData.resize(m_noiseParams.width * m_noiseParams.height);

	for (int y = 0; y < m_noiseParams.height; y++)
	{
		for (int x = 0; x < m_noiseParams.width; x++)
		{
			float newX = (float)x / m_noiseParams.width;
			float newY = (float)y / m_noiseParams.height;

			float scaledX = (newX * m_noiseParams.scale) + m_noiseParams.offset.x;
			float scaledY = (newY * m_noiseParams.scale) + m_noiseParams.offset.y;

			float noiseValue = m_noise.GetNoise(scaledX, scaledY);
			noiseValue = (noiseValue + 1.0f) * 0.5f;
			noiseValue *= m_noiseParams.amplitude;

			unsigned char pixelValue = (unsigned char)std::clamp(noiseValue * 255.0f, 0.0f, 255.0f);

			size_t pixelIndex = (y * m_noiseParams.width + x) * 4;
			m_pixels[pixelIndex] = pixelValue;		//R
			m_pixels[pixelIndex + 1] = pixelValue;	//G
			m_pixels[pixelIndex + 2] = pixelValue;	//B
			m_pixels[pixelIndex + 3] = 255;			//A

			size_t noiseDataIndex = y * m_noiseParams.width + x;
			m_noiseData[noiseDataIndex] = noiseValue;
		}
	}
}

void pwg::NoiseTexture::UploadToGPU()
{

	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_noiseParams.width, m_noiseParams.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}
