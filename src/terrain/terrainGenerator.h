#include "terrain.h"
#ifndef SRC_TERRAIN_TERRAIN_GENERATOR_H
#define SRC_TERRAIN_TERRAIN_GENERATOR_H



namespace pwg
{
	struct TerrainNoiseSettings;

	struct TerrainTextures
	{
		std::shared_ptr<Texture> heightmap;
		std::shared_ptr<Texture> normalmap;
		std::shared_ptr<Texture> splatmap;
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator(std::shared_ptr<ComputeShader> noiseComputeShader);
		~TerrainGenerator() = default;

		std::shared_ptr<TerrainTextures> GenerateTerrain(TerrainNoiseSettings& noiseSettings, int size);

	private:
		std::shared_ptr<ComputeShader> m_noiseComputeShader;

		std::shared_ptr<TerrainTextures> m_textures;
		bool m_texturesCreated{ false };

		unsigned int m_heightmapID;
		unsigned int m_normalmapID;
		unsigned int m_splatmapID;

	};
}

#endif // !SRC_TERRAIN_TERRAIN_GENERATOR_H
