#include "planeMeshSystem.h"

namespace pwg::systems
{
	void PlaneMeshSystem::Update(entt::registry& registry, MeshManager& meshManager)
	{
		auto view = registry.view<components::PlaneMeshComponent, components::MeshComponent>();
		for (auto [entity, plane, mesh] : view.each())
		{
			auto& planeMesh = plane;
			auto& meshComp = mesh;

			if (planeMesh.changed)
			{
				meshComp.meshID = meshManager.CreateMesh(planeMesh.size, "PlaneMesh");
			}
			planeMesh.changed = false;
			
		}
	}
}

