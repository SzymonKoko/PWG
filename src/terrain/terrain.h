#ifndef SRC_TERRAIN_TERRAIN_H
#define SRC_TERRAIN_TERRAIN_H

#include <string>
#include <memory>
#include "resources/resourceManager.h"
#include "graphics/renderer/renderer.h"
#include "graphics/renderer/irenderable.h"
#include "terrain/terrainGenerator.h"

namespace pwg
{
	struct TerrainNoiseSettings
	{
		int noiseType = 0;
		float amplitude = 0.01f;
		float frequency = 0.01f;
		float scale = 200.0f;
		float lacunarity = 2.0f;
		float persistance = 0.5f;
		int octaves = 1;
		int seed = 1000;
		glm::vec2 offset{ 0.0f, 0.0f };

		bool dirty = true;
	};

	/**
	 * @brief Handles terrain generation, noise application, and terrain layers.
	 */
	class Terrain
	{
	public:
		/**
		 * @brief Constructs a terrain instance.
		 * @param registry Reference to the ECS registry to store entities.
		 * @param resourceManager Shared pointer to resource manager for meshes/textures.
		 * @param size Size of the terrain (width/height in units or grid points).
		 */
		Terrain(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, std::shared_ptr<ComputeShader> noiseComputeShader);

		/**
		 * @brief Destructor. Cleans up terrain resources.
		 */
		~Terrain();

		void Update();

		void Draw(Renderer& renderer);

		/**
		 * @brief Returns the terrain mesh.
		 * @return Shared pointer to the Mesh instance.
		 */
		std::shared_ptr<Mesh> GetMesh();

		/**
		 * @brief Returns the size of the terrain.
		 * @return Terrain size (width/height).
		 */
		int GetSize();

		TerrainNoiseSettings& GetNoiseSettings();

	private:
		int m_size;														/**< Terrain size (width/height). */
		std::shared_ptr<Mesh> m_mesh;
		std::shared_ptr<Material> m_material;
		std::shared_ptr<TerrainTextures> m_terrainTextures;
		std::unique_ptr<TerrainGenerator> m_terrainGenerator;
		TerrainNoiseSettings m_terrainNoiseSettings;
	};
}
#endif // !SRC_TERRAIN_TERRAIN_H
