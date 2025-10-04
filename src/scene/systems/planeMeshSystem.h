#ifndef SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM
#define SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM

#include <entt/entt.hpp>
#include "core/ecs/entity.h"
#include "scene/components/meshComponent.h"
#include "scene/components/planeMeshComponent.h"
#include "graphics/mesh/meshManager.h"


namespace pwg::systems
{
	class PlaneMeshSystem
	{
	public:
		PlaneMeshSystem() = default;
		~PlaneMeshSystem() = default;

		static void Update(entt::registry& registry, MeshManager& meshManager);
	private:

	};
}
#endif // !SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM
