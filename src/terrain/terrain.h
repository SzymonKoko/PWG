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

namespace pwg
{
	class Terrain
	{
	public:
		Terrain(entt::registry& registry, std::shared_ptr<ResourceManager> resourceManager, int size);
		~Terrain();

		void ApplyNoise();

		void Update();

		std::shared_ptr<Mesh> GetMesh();
		std::shared_ptr<NoiseTexture> GetNoiseTexture() { return m_noiseTexture; }
	private:
		int m_size;

		entt::registry& m_registry;
		std::shared_ptr<NoiseTexture> m_noiseTexture;
		std::shared_ptr<ResourceManager> m_resourceManager;
	};
}
#endif // !SRC_TERRAIN_TERRAIN_H
