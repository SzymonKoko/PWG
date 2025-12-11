#include "terrainGenerator.h"

namespace pwg
{
	TerrainGenerator::TerrainGenerator(std::shared_ptr<ComputeShader> noiseComputeShader)
		: m_noiseComputeShader(noiseComputeShader)
	{

	}

	std::shared_ptr<TerrainTextures> TerrainGenerator::GenerateTerrain(TerrainNoiseSettings& noiseSettings, int size)
	{
		//Initialize empty textures
		m_textures = std::make_shared<TerrainTextures>();

		if (!m_texturesCreated)
		{
			m_textures->heightmap = std::make_shared<Texture>(size, size);
			m_textures->normalmap = std::make_shared<Texture>(size, size);
			m_textures->splatmap = std::make_shared<Texture>(size, size);

			m_heightmapID = m_textures->heightmap->GetTextureID();
			m_normalmapID = m_textures->normalmap->GetTextureID();
			m_splatmapID = m_textures->splatmap->GetTextureID();

			m_texturesCreated = true;
		}

		//Activate shader
		m_noiseComputeShader->ActivateShader();

		//Fill shader uniforms with default settings
		m_noiseComputeShader->SetUniformFloat("amplitude", noiseSettings.amplitude);
		m_noiseComputeShader->SetUniformFloat("frequency", noiseSettings.frequency);
		m_noiseComputeShader->SetUniformFloat("scale", noiseSettings.scale);
		m_noiseComputeShader->SetUniformFloat("persistance", noiseSettings.persistance);
		m_noiseComputeShader->SetUniformFloat("lacunarity", noiseSettings.lacunarity);
		m_noiseComputeShader->SetUniformVec2("offset", noiseSettings.offset);
		m_noiseComputeShader->SetUniformInt("noiseType", noiseSettings.noiseType);
		m_noiseComputeShader->SetUniformInt("octaves", noiseSettings.octaves);
		m_noiseComputeShader->SetUniformInt("seed", noiseSettings.seed);
		m_noiseComputeShader->SetUniformInt("size", size);

		//Bind textures to slots
		m_noiseComputeShader->BindImage(0, m_heightmapID, GL_WRITE_ONLY, GL_R32F);
		m_noiseComputeShader->BindImage(1, m_normalmapID, GL_WRITE_ONLY, GL_RGBA16F);
		m_noiseComputeShader->BindImage(2, m_splatmapID, GL_WRITE_ONLY, GL_RGBA8);

		//Dispatch compute shader
		m_noiseComputeShader->DispatchForTexture(size, size);
		m_noiseComputeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		return m_textures;
	}

}
