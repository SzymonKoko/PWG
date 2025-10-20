#ifndef SRC_TERRAIN_TERRAIN_H
#define SRC_TERRAIN_TERRAIN_H

#include <string>
#include <entt/entt.hpp>
#include <memory>
#include "core/ecs/entity.h"
#include "scene/components/meshComponent.h"
#include "scene/components/planeMeshComponent.h"
#include "scene/systems/planeMeshSystem.h"
#include "procedural/noiseDeformer.h"
#include "procedural/noiseTexture.h"
#include "resources/resourceManager.h"
#include "terrainLayer.h"

namespace pwg
{
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
		Terrain(entt::registry& registry, std::shared_ptr<ResourceManager> resourceManager, int size);

		/**
		 * @brief Destructor. Cleans up terrain resources.
		 */
		~Terrain();

		/**
		 * @brief Updates terrain logic, including noise application and layers.
		 */
		void Update();

		/**
		 * @brief Adds a terrain layer to the terrain.
		 * @param terrainLayer TerrainLayer instance to add.
		 */
		void AddLayer(const TerrainLayer& terrainLayer);

		/**
		 * @brief Removes a terrain layer by its name.
		 * @param name Name of the terrain layer to remove.
		 */
		void RemoveLayer(std::string name);

		/**
		 * @brief Returns the terrain mesh.
		 * @return Shared pointer to the Mesh instance.
		 */
		std::shared_ptr<Mesh> GetMesh();

		/**
		 * @brief Returns the terrain noise texture.
		 * @return Shared pointer to the NoiseTexture instance.
		 */
		std::shared_ptr<NoiseTexture> GetNoiseTexture() { return m_noiseTexture; }

		/**
		 * @brief Returns the size of the terrain.
		 * @return Terrain size (width/height).
		 */
		int GetSize();

	private:

		/**
		 * @brief Applies the noise data to the terrain mesh.
		 */
		void ApplyNoise();

		/**
		 * @brief Applies all added terrain layers to modify the mesh.
		 */
		void ApplyLayers();

	private:
		int m_size;														/**< Terrain size (width/height). */
		std::unordered_map<std::string, TerrainLayer> m_terrainLayers;	/**< Map of terrain layers by name. */
	
		entt::registry& m_registry;										/**< Reference to ECS registry. */
		std::shared_ptr<NoiseTexture> m_noiseTexture;					/**< Noise texture used for terrain deformation. */
		std::shared_ptr<ResourceManager> m_resourceManager;				/**< Shared pointer to resource manager. */
	};
}
#endif // !SRC_TERRAIN_TERRAIN_H
