#ifndef SRC_TERRAIN_TERRAIN_GENERATOR_H
#define SRC_TERRAIN_TERRAIN_GENERATOR_H

#include "graphics/texture/texture.h"
#include "graphics/shader/shader.h"
#include "graphics/shader/computeShader.h"
#include "graphics/common/GL_Types.h"

namespace pwg
{
	struct TerrainNoiseSettings;
	struct TerrainLayer;

	struct TerrainTextures
	{
		std::shared_ptr<Texture> heightmap;
		std::shared_ptr<Texture> normalmap;
		std::shared_ptr<Texture> splatmap;
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator(std::shared_ptr<ComputeShader> heightmapComputeShader, std::shared_ptr<ComputeShader> normalmapComputeShader, std::shared_ptr<ComputeShader> splatmapComputeShader);
		~TerrainGenerator() = default;

		std::shared_ptr<TerrainTextures> GenerateTerrain(TerrainNoiseSettings& noiseSettings, int size, std::vector<TerrainLayer> layers);

		std::vector<float> GetCPUHeightmap();

	private:
		void GenerateHeightmap(TerrainNoiseSettings& noiseSettings);
		void GenerateNormalmap(float amlitude);
		void GenerateSplatmap(std::vector<TerrainLayer> layers, float amplitude);

		std::shared_ptr<ComputeShader> m_heightmapComputeShader;
		std::shared_ptr<ComputeShader> m_normalmapComputeShader;
		std::shared_ptr<ComputeShader> m_splatmapComputeShader;

		std::shared_ptr<TerrainTextures> m_textures;
		bool m_texturesCreated{ false };

		unsigned int m_heightmapID;
		unsigned int m_normalmapID;
		unsigned int m_splatmapID;

		int m_size{ 0 };
		std::vector<float> m_cpuHeightmap;

	};
}

#endif // !SRC_TERRAIN_TERRAIN_GENERATOR_H
