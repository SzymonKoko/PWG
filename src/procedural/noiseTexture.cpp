#include <glad/glad.h>
#include "noiseTexture.h"
#include <iostream>
#include "core/logger/logger.h"

pwg::NoiseTexture::NoiseTexture(int terrainSize)
{
	m_noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_OpenSimplex2);
	m_noiseParams.size = terrainSize;

	if (m_textureID != 0)
		glDeleteTextures(1, &m_textureID);

	glGenTextures(1, &m_textureID);

	UpdateNoiseData(m_noiseParams);
	PWG_INFO("Noise texture has been created ({0}x{0}, id={1})",terrainSize, m_textureID);
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

void pwg::NoiseTexture::SetOctaves(int octaves)
{
	m_noiseParams.octaves = octaves;
}

void pwg::NoiseTexture::SetPersistance(float persistance)
{
	m_noiseParams.persistance = persistance;
}

void pwg::NoiseTexture::SetLacunarity(float lacunarity)
{
	m_noiseParams.lacunarity = lacunarity;
}

void pwg::NoiseTexture::SetNoiseType(FastNoiseLite::NoiseType noiseType)
{
	m_noise.SetNoiseType(noiseType);
}

void pwg::NoiseTexture::SetFractalType(FastNoiseLite::FractalType fractalType)
{
	m_noise.SetFractalType(fractalType);
}

void pwg::NoiseTexture::SetFractalOctaves(int octaves)
{
	m_noise.SetFractalOctaves(octaves);
}

void pwg::NoiseTexture::SetFractalLacunarity(float lacunarity)
{
	m_noise.SetFractalLacunarity(lacunarity);
}

void pwg::NoiseTexture::SetFractalGain(float gain)
{
	m_noise.SetFractalGain(gain);
}

void pwg::NoiseTexture::SetFractalWeightedStrength(float strength)
{
	m_noise.SetFractalWeightedStrength(strength);
}

void pwg::NoiseTexture::SetFractalPingPongStrength(float strength)
{
	m_noise.SetFractalPingPongStrength(strength);
}

void pwg::NoiseTexture::SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction cellularDistFunction)
{
	m_noise.SetCellularDistanceFunction(cellularDistFunction);
}

void pwg::NoiseTexture::SetCellularReturnType(FastNoiseLite::CellularReturnType cellularReturnType)
{
	m_noise.SetCellularReturnType(cellularReturnType);
}

void pwg::NoiseTexture::SetCellularJitter(float jitter)
{
	m_noise.SetCellularJitter(jitter);
}

void pwg::NoiseTexture::GenerateNoiseData()
{
	m_pixels.resize(m_noiseParams.size * m_noiseParams.size * 4);
	m_noiseData.resize(m_noiseParams.size * m_noiseParams.size);

	for (int y = 0; y < m_noiseParams.size; y++)
	{
		for (int x = 0; x < m_noiseParams.size; x++)
		{
			float noiseHeight = 0.0f;
			float amplitude = m_noiseParams.amplitude;
			float frequency = m_noiseParams.frequency;

			for (int i = 0; i < m_noiseParams.octaves; i++)
			{
				float newX = (float)x / m_noiseParams.size;
				float newY = (float)y / m_noiseParams.size;

				float scaledX = (newX * m_noiseParams.scale * frequency) + m_noiseParams.offset.x;
				float scaledY = (newY * m_noiseParams.scale * frequency) + m_noiseParams.offset.y;

				float noiseValue = m_noise.GetNoise(scaledX, scaledY);
				noiseValue = (noiseValue + 1.0f) * 0.5f;

				noiseHeight += noiseValue * amplitude;

				amplitude *= m_noiseParams.persistance;
				frequency *= m_noiseParams.lacunarity;
			}

			unsigned char pixelValue = (unsigned char)std::clamp(noiseHeight * 255.0f, 0.0f, 255.0f);

			

			size_t pixelIndex = (y * m_noiseParams.size + x) * 4;
			m_pixels[pixelIndex + 0] = pixelValue;
			m_pixels[pixelIndex + 1] = pixelValue;
			m_pixels[pixelIndex + 2] = pixelValue;
			m_pixels[pixelIndex + 3] = 255;

			size_t noiseDataIndex = y * m_noiseParams.size + x;
			m_noiseData[noiseDataIndex] = noiseHeight;
		}
	}
}

void pwg::NoiseTexture::UploadToGPU()
{

	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_noiseParams.size, m_noiseParams.size, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}
