#ifndef SRC_TERRAIN_TERRAIN_H
#define SRC_TERRAIN_TERRAIN_H

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "resources/resourceManager.h"
#include "graphics/renderer/irenderable.h"
#include "terrain/terrainGenerator.h"
#include "terrain/terrainLayersManager.h"
#include "graphics/shader/computeShader.h"
#include "terrain/chunk/chunkManager.h"
#include "terrain/debug/terrainDebug.h"

namespace pwg
{

	/**
	 * @brief Struct containing terrain noise parameters for procedural generation.
	 */
	struct TerrainNoiseSettings
	{
		int noiseType = 0;
		float amplitude = 200.0f;
		float frequency = 0.01f;
		float scale = 200.0f;
		int seed = 1000;
		glm::vec2 offset{ 0.0f, 0.0f };

		// Fractal noise parameters
		int fractalType = 0;
		float lacunarity = 2.0f;
		float persistance = 0.5f;
		int octaves = 1;

		// Domain warp parameters
		bool domainWarp = false;
		int domainWarpType = 0;
		float domainWarpAmplitude = 0.0f;

		bool dirty = true; /**< Flag indicating if noise needs to be regenerated. */
	};

	/**
	 * @brief Holds compute shaders used for terrain generation (heightmap, normalmap, splatmap).
	 */
	struct TerrainComputeShaders
	{
		std::shared_ptr<ComputeShader> heightmapShader;
		std::shared_ptr<ComputeShader> normalmapShader;
		std::shared_ptr<ComputeShader> splatmapShader;
	};

	/**
	 * @brief Class representing the terrain.
	 * Handles mesh generation, noise application, compute shaders, and terrain layers.
	 */
	class Terrain : public IRenderable
	{
	public:

		/**
		 * @brief Constructs a Terrain instance.
		 * @param mesh Shared pointer to a base mesh (usually a plane).
		 * @param material Shared pointer to the terrain material.
		 * @param computeShaders Reference to compute shaders used for terrain generation.
		 */
		Terrain(std::shared_ptr<ResourceManager> resourceManager);

		/**
		 * @brief Destructor. Cleans up terrain resources.
		 */
		~Terrain();

		/**
		 * @brief Updates the terrain per frame.
		 * @param dt Delta time since last frame.
		 */
		void Update(float dt, std::shared_ptr<ICamera> camera) override;

		/**
		 * @brief Draws the terrain using the provided renderer.
		 * @param renderer Reference to the renderer.
		 */
		void Draw(Renderer& renderer) override;

		/**
		 * @brief Returns the terrain mesh.
		 * @return Shared pointer to the Mesh instance.
		 */
		std::shared_ptr<Mesh> GetMesh() override;

		/**
		 * @brief Returns the terrain material.
		 * @return Shared pointer to the Material instance.
		 */
		std::shared_ptr<Material> GetMaterial() override;

		std::shared_ptr<ChunkManager> GetChunkManager();

		/**
		 * @brief Returns the terrain size (number of vertices per side).
		 */
		int GetSize();

		/**
		 * @brief Accessor for terrain noise settings.
		 */
		TerrainNoiseSettings& GetNoiseSettings();

		/**
		 * @brief Returns terrain layers managed by TerrainLayersManager.
		 */
		std::vector<TerrainLayer>& GetTerrainLayers();

		void SetTerrainTextures(std::unordered_map<std::string, std::shared_ptr<Texture>>& textures);

		/**
		 * @brief Returns the terrain height at a given (x, z) coordinate.
		 */
		float GetTerrainHeightAt(int x, int z);

		std::shared_ptr<TerrainDebug> GetTerrainDebug();

	private:

		struct TerrainTextures
		{
			std::shared_ptr<Texture> heightmap;
			std::shared_ptr<Texture> normalmap;
			std::shared_ptr<Texture> splatmap;
		};

		int m_size;														/**< Terrain size (width/height). */
		std::shared_ptr<Mesh> m_mesh;									/**< Mesh representing terrain surface. */
		std::shared_ptr<Material> m_material;							/**< Terrain material (shader + textures). */
		std::unique_ptr<TerrainLayersManager> m_terrainLayersManager;	/**< Manages terrain layers and splat maps. */
		TerrainNoiseSettings m_terrainNoiseSettings;					/**< Noise settings for procedural generation. */
		TerrainTextures m_terrainTextures;
		TerrainSettings m_terrainSettings;
		std::shared_ptr<ChunkManager> m_chunkManager;
		std::shared_ptr<TerrainDebug> m_terrainDebug;
	};
} // namespace pwg

#endif // !SRC_TERRAIN_TERRAIN_H
