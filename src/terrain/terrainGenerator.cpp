#include "terrainGenerator.h"
#include "terrain/terrain.h"

namespace pwg
{
	TerrainGenerator::TerrainGenerator(std::shared_ptr<ComputeShader> heightmapComputeShader, std::shared_ptr<ComputeShader> normalmapComputeShader, std::shared_ptr<ComputeShader> splatmapComputeShader)
		: m_heightmapComputeShader(heightmapComputeShader),
		  m_normalmapComputeShader(normalmapComputeShader),
		  m_splatmapComputeShader(splatmapComputeShader)
	{

	}

	void TerrainGenerator::GenerateHeightmap(TerrainNoiseSettings& noiseSettings)
	{
		//Activate shader
		m_heightmapComputeShader->ActivateShader();

		//Fill shader uniforms with default settings
		m_heightmapComputeShader->SetUniformFloat("amplitude", noiseSettings.amplitude);
		m_heightmapComputeShader->SetUniformFloat("frequency", noiseSettings.frequency);
		m_heightmapComputeShader->SetUniformFloat("scale", noiseSettings.scale);
		m_heightmapComputeShader->SetUniformFloat("persistance", noiseSettings.persistance);
		m_heightmapComputeShader->SetUniformFloat("lacunarity", noiseSettings.lacunarity);
		m_heightmapComputeShader->SetUniformVec2("offset", noiseSettings.offset);
		m_heightmapComputeShader->SetUniformInt("noiseType", noiseSettings.noiseType);
		m_heightmapComputeShader->SetUniformInt("octaves", noiseSettings.octaves);
		m_heightmapComputeShader->SetUniformInt("seed", noiseSettings.seed);
		m_heightmapComputeShader->SetUniformInt("size", m_size);

		//Bind textures to slots
		m_heightmapComputeShader->BindImage(0, m_heightmapID, GL_WRITE_ONLY, GL_R32F);

		//Dispatch compute shader
		m_heightmapComputeShader->DispatchForTexture(m_size, m_size);
		m_heightmapComputeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	void TerrainGenerator::GenerateNormalmap(float amlitude)
	{
		m_normalmapComputeShader->ActivateShader();

		m_normalmapComputeShader->SetUniformFloat("u_amplitude", amlitude);

		m_normalmapComputeShader->BindImage(0, m_heightmapID, GL_READ_ONLY, GL_R32F);
		m_normalmapComputeShader->BindImage(1, m_normalmapID, GL_WRITE_ONLY, GL_RGBA16F);

		m_normalmapComputeShader->DispatchForTexture(m_size, m_size);
		m_normalmapComputeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	void TerrainGenerator::GenerateSplatmap(std::vector<TerrainLayer> layers, float amplitude)
	{
		unsigned int terrainLayersSSBO;
		glCreateBuffers(1, &terrainLayersSSBO);

		glNamedBufferStorage(terrainLayersSSBO, sizeof(TerrainLayer) * layers.size(), layers.data(), GL_DYNAMIC_STORAGE_BIT);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, terrainLayersSSBO);

		m_splatmapComputeShader->ActivateShader();

		m_splatmapComputeShader->SetUniformInt("u_layersCount", layers.size());
		m_splatmapComputeShader->SetUniformFloat("u_amplitude", amplitude);

		m_splatmapComputeShader->BindImage(0, m_heightmapID, GL_READ_ONLY, GL_R32F);
		m_splatmapComputeShader->BindImage(1, m_normalmapID, GL_READ_ONLY, GL_RGBA16F);
		m_splatmapComputeShader->BindImage(2, m_splatmapID, GL_WRITE_ONLY, GL_RGBA8);

		m_splatmapComputeShader->DispatchForTexture(m_size, m_size);
		m_splatmapComputeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	std::shared_ptr<TerrainTextures> TerrainGenerator::GenerateTerrain(TerrainNoiseSettings& noiseSettings, int size, std::vector<TerrainLayer> layers)
	{
		m_size = size;
		m_cpuHeightmap.resize(m_size * m_size);

		//Initialize empty textures
		m_textures = std::make_shared<TerrainTextures>();

		if (!m_texturesCreated)
		{
			m_textures->heightmap = std::make_shared<Texture>(size, size, ToGL(TextureFormats::R32F));
			m_textures->normalmap = std::make_shared<Texture>(size, size, ToGL(TextureFormats::RGBA16F));
			m_textures->splatmap = std::make_shared<Texture>(size, size, ToGL(TextureFormats::RGBA8));

			m_textures->heightmap->SetTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

			m_heightmapID = m_textures->heightmap->GetTextureID();
			m_normalmapID = m_textures->normalmap->GetTextureID();
			m_splatmapID = m_textures->splatmap->GetTextureID();

			m_texturesCreated = true;
		}

		GenerateHeightmap(noiseSettings);

		if (m_texturesCreated)
		{
			glBindTexture(GL_TEXTURE_2D, m_heightmapID);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, m_cpuHeightmap.data());
		}
		
		GenerateNormalmap(noiseSettings.amplitude);
		GenerateSplatmap(layers, noiseSettings.amplitude);



		
		
		return m_textures;
	}

	std::vector<float> TerrainGenerator::GetCPUHeightmap()
	{
		return m_cpuHeightmap;
	}

}
