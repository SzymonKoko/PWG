#ifndef SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM
#define SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM

#include <entt/entt.hpp>
#include "core/ecs/entity.h"
#include "scene/components/meshComponent.h"
#include "scene/components/planeMeshComponent.h"
#include "graphics/mesh/meshManager.h"

namespace pwg::systems
{
	/**
	 * @brief System that updates plane meshes in the ECS registry.
	 * Synchronizes PlaneMeshComponent data with MeshManager to ensure meshes are up-to-date.
	 */
	class PlaneMeshSystem
	{
	public:

		/**
		 * @brief Default constructor.
		 */
		PlaneMeshSystem() = default;

		/**
		 * @brief Default destructor.
		 */
		~PlaneMeshSystem() = default;

		/**
		 * @brief Updates all entities with PlaneMeshComponent in the registry.
		 * If the plane mesh has changed, the mesh is updated via the MeshManager.
		 * @param registry Reference to the ECS registry.
		 * @param meshManager Reference to the MeshManager used to manage mesh resources.
		 */
		static void Update(entt::registry& registry, MeshManager& meshManager);

	private:
		// No private members for now.
	};
} // namespace pwg::systems

#endif // !SRC_SCENE_SYSTEMS_PLANE_MESH_SYSTEM
